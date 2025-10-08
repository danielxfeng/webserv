#include "WebServ.hpp"

volatile std::sig_atomic_t stopFlag = 0;

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw WebServErr::SysCallErrException("fcntl(F_GETFL) failed");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw WebServErr::SysCallErrException("fcntl(F_SETFL) failed");

    int fdflags = fcntl(fd, F_GETFD, 0);
    if (fdflags == -1)
        throw WebServErr::SysCallErrException("fcntl(F_GETFD) failed");

    if (fcntl(fd, F_SETFD, fdflags | FD_CLOEXEC) == -1)
        throw WebServErr::SysCallErrException("fcntl(F_SETFD) failed");
}

int listenToPort(unsigned int port)
{
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
        throw WebServErr::SysCallErrException("socket creation failed");

    setNonBlocking(serverFd);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw WebServErr::SysCallErrException("setsockopt(SO_REUSEADDR) failed");

    if (bind(serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        throw WebServErr::SysCallErrException("bind failed");

    if (listen(serverFd, SOMAXCONN) == -1)
        throw WebServErr::SysCallErrException("listen failed");

    LOG_INFO("Listening on port:", port);
    return serverFd;
}

int acceptNewConnection(int listenFd)
{
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int connClientFd = accept(listenFd, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (connClientFd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1; // No more connections to accept
        if (errno == ECONNABORTED || errno == EINTR)
            return -1; // Connection aborted or interrupted, ignore this error
        throw WebServErr::SysCallErrException("accept failed");
    }

    setNonBlocking(connClientFd);

    LOG_INFO("Accepted new connection, fd:", connClientFd);
    return connClientFd;
}

void handleSignal(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        stopFlag = 1;
    }
}

void WebServ::timeoutKiller(const std::unordered_map<int, Server *> &serverMap)
{
    for (const auto &server : serverMap)
        handleServerMsg(server.second->timeoutKiller(), server.second);
}

WebServ::WebServ(const std::string &conf_file) : epoll_(EpollHelper())
{
    // Load the configuration from the file.
    try
    {
        config_ = std::move(Config::parseConfigFromFile(conf_file));
        LOG_INFO("Configuration loaded from file:", conf_file);

        // Setup signal handlers for graceful shutdown.
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        std::unordered_map<size_t, std::vector<t_server_config>> ports_map;

        for (auto it = config_.servers.begin(); it != config_.servers.end(); ++it)
        {
            auto port = it->port;
            if (ports_map.contains(port))
                ports_map.at(port).push_back(*it);
            else
                ports_map.emplace(port, std::vector<t_server_config>{*it});
        }

        for (auto &kv : ports_map)
            servers_.push_back(Server(*this, epoll_, kv.second));
    }
    catch (...)
    {
        throw std::runtime_error("Failed to initialize WebServ with config file: " + conf_file + " - " + std::string(std::current_exception().__cxa_exception_type()->name()));
    }
}

void WebServ::eventLoop()
{
    struct epoll_event events[config_.max_poll_events];

    LOG_INFO("Server started.", "");
    for (auto it = servers_.begin(); it != servers_.end(); ++it)
    {
        RaiiFd serverFd = RaiiFd(epoll_, listenToPort(it->getConfig(0).port)); // every config in one server should has same port
        serverFd.addToEpoll();
        fds_.push_back(std::make_shared<RaiiFd>(std::move(serverFd)));
        server_map_[fds_.back()->get()] = &(*it);
    }

    while (!stopFlag)
    {
        int numEvents = epoll_wait(epoll_.getEpollFd(), events, config_.max_poll_events, config_.max_poll_timeout);
        if (numEvents == -1)
        {
            if (errno == EINTR)
                continue; // Interrupted by signal, retry
            throw WebServErr::SysCallErrException("epoll_wait failed");
        }

        for (int i = 0; i < numEvents; ++i)
        {
            const auto server = server_map_.find(events[i].data.fd);
            const bool isNewConn = server != server_map_.end() && (events[i].events & EPOLLIN);
            if (isNewConn)
            {
                if (conn_map_.size() >= MAX_CONN)
                    continue;

                int connClientFd = acceptNewConnection(server->first);
                if (connClientFd == -1)
                {
                    continue;
                }

                fds_.push_back(std::make_shared<RaiiFd>(epoll_, connClientFd));
                fds_.back()->addToEpoll();
                conn_map_[fds_.back()->get()] = server->second;
                server->second->addConn(fds_.back()->get());
            }
            else
            {
                const auto connServer = conn_map_.find(events[i].data.fd);
                if (connServer == conn_map_.end())
                {
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    auto msg = connServer->second->scheduler(connServer->first, READ_EVENT);
                    handleServerMsg(msg, connServer->second);
                }
                if (events[i].events & EPOLLOUT)
                {
                    auto msg = connServer->second->scheduler(connServer->first, WRITE_EVENT);
                    handleServerMsg(msg, connServer->second);
                }
                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    auto msg = connServer->second->scheduler(connServer->first, ERROR_EVENT);
                    handleServerMsg(msg, connServer->second);
                }
            }
        }
        timeoutKiller(server_map_);
    }
    LOG_INFO("Server shutting down gracefully.", "");
}

void WebServ::handleServerMsg(const t_msg_from_serv &msg, Server *server)
{
    for (const auto &fd : msg.fds_to_register)
    {
        addFdToEpoll(fd, server);
    }
    for (const auto &fd : msg.fds_to_unregister)
    {
        server_map_.erase(fd);
        conn_map_.erase(fd);
        fds_.remove_if([&fd](const std::shared_ptr<RaiiFd> &rfd)
                       { return rfd->get() == fd; });
    }
}

void WebServ::addFdToEpoll(const std::shared_ptr<RaiiFd> fd, Server *server)
{
    fds_.push_back(fd);
    fds_.back()->addToEpoll();
    conn_map_[fds_.back()->get()] = server;
}

WebServ::~WebServ()
{
    LOG_INFO("Closing down server.", "");
}
