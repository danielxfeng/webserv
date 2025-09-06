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

    LOG_INFO("Set fd to non-blocking:", fd);
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

void timeoutKiller(const std::unordered_map<int, Server *> &serverMap)
{
    for (const auto &server : serverMap)
        server.second->timeoutKiller();
}

WebServ::WebServ(const std::string &conf_file) : epoll_(EpollHelper())
{
    // Load the configuration from the file.
    config_ = std::move(Config::parseConfigFromFile(conf_file));
    LOG_INFO("Configuration loaded from file:", conf_file);

    // Setup signal handlers for graceful shutdown.
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // Create server instances based on the configuration.
    for (const auto &server_conf : config_.servers)
    {
        servers_.push_back(Server(server_conf.second));
        LOG_INFO("Created server instance for:", server_conf.first);
    }
}

void WebServ::eventLoop()
{
    struct epoll_event events[config_.max_poll_events];

    LOG_INFO("Server started.", "");
    for (auto it = servers_.begin(); it != servers_.end(); ++it)
    {
        RaiiFd serverFd = RaiiFd(epoll_, listenToPort(it->getConfig().port));
        LOG_INFO("Server listening to port:", serverFd);
        serverFd.addToEpoll();
        fds_.push_back(std::move(serverFd));
        server_map_[fds_.back().get()] = &(*it);
        LOG_INFO("Mapped listening fd to server instance:", fds_.back().get());
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
                int connClientFd = acceptNewConnection(server->first);
                if (connClientFd == -1)
                {
                    LOG_ERROR("No new connection to accept or error occurred, server fd: ", server->first);
                    continue;
                }

                fds_.push_back(RaiiFd(epoll_, connClientFd));
                fds_.back().addToEpoll();
                conn_map_[fds_.back().get()] = server->second;
                LOG_INFO("Mapped connection fd to server instance:", fds_.back().get());
            }
            else
            {
                const auto connServer = conn_map_.find(events[i].data.fd);
                if (connServer == conn_map_.end())
                {
                    LOG_ERROR("Connection not found", events[i].data.fd);
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    auto msg = connServer->second->handleDataIn(connServer->first);
                    handleServerMsg(msg, connServer->second);
                }
                if (events[i].events & EPOLLOUT)
                {
                    auto msg = connServer->second->handleDataOut(connServer->first);
                    handleServerMsg(msg, connServer->second);
                }
                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    auto msg = connServer->second->handleDataEnd(connServer->first);
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
        fds_.push_back(std::move(fd));
        fds_.back().addToEpoll();
        conn_map_[fds_.back().get()] = server;
        LOG_INFO("Registered fd to epoll:", fds_.back().get());
    }
    for (const auto &fd : msg.fds_to_unregister)
    {
        server_map_.erase(fd);
        conn_map_.erase(fd);
        fds_.remove_if([&fd](const RaiiFd &rfd)
                       { return rfd.get() == fd; });
        LOG_INFO("Unregistered fd from epoll and removed from maps:", fd);
    }
}

WebServ::~WebServ()
{
    LOG_INFO("Closing down server.", "");
}
