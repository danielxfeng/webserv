#include "WebServ.hpp"
#include "LogSys.hpp"

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw WebServErr::SysCallErrException("fcntl(F_GETFL) failed");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw WebServErr::SysCallErrException("fcntl(F_SETFL) failed");
}

int listenToPort(unsigned int port)
{
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
        throw WebServErr::SysCallErrException("socket creation failed");

    setNonBlocking(serverFd);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        throw WebServErr::SysCallErrException("bind failed");

    if (listen(serverFd, 10) == -1)
        throw WebServErr::SysCallErrException("listen failed");

    return serverFd;
}

void registerToEpoll(int epollFd, int fd, uint32_t events)
{
    struct epoll_event event{};
    event.events = events;
    event.data.fd = fd;

    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event) == -1)
    {
        if (errno == ENOENT)
        {
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1)
                throw WebServErr::SysCallErrException("epoll_ctl ADD failed");
        }
        else
            throw WebServErr::SysCallErrException("epoll_ctl MOD failed");
    }
}

void timeoutKiller(const std::unordered_map<int, Server *> &serverMap)
{
    for (const auto &server : serverMap)
        server.second->timeoutKiller();
}

WebServ::WebServ(const std::string &conf_file) : epollFd_(-1)
{
    // Load the configuration from the file.
    config_ = std::move(Config::parseConfigFromFile(conf_file));

    // Create server instances based on the configuration.
    for (const auto &server_conf : config_.servers)
        serverVec_.push_back(Server(server_conf.second));

    // Initialize epoll.
    epollFd_ = epoll_create(1);
    if (epollFd_ == -1)
        throw WebServErr::SysCallErrException("epoll_create failed");
}

void WebServ::eventLoop()
{
    struct epoll_event events[config_.max_poll_events];

    LOG_INFO("Server started.", "");
    for (auto it = serverVec_.begin(); it != serverVec_.end(); ++it)
    {
        int serverFd = listenToPort(it->getConfig().port);
        LOG_INFO("Server listening to port:", serverFd);
        registerToEpoll(epollFd_, serverFd, IN_FLAGS);
        serverMap_[serverFd] = &(*it);
    }

    while (true)
    {
        int numEvents = epoll_wait(epollFd_, events, config_.max_poll_events, config_.max_poll_timeout);
        if (numEvents == -1)
            throw WebServErr::SysCallErrException("epoll_wait failed");

        for (int i = 0; i < numEvents; ++i)
        {
            const auto server = serverMap_.find(events[i].data.fd);
            const bool isNewConn = server != serverMap_.end() && (events[i].events & EPOLLIN);
            if (isNewConn)
            {
                struct sockaddr_in clientAddress;
                socklen_t clientAddressLength = sizeof(clientAddress);
                int connClientFd = accept(events[i].data.fd, (struct sockaddr *)&clientAddress, &clientAddressLength);
                if (connClientFd == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue; // No more connections to accept
                    if (errno == ECONNABORTED || errno == EINTR)
                        continue; // Connection aborted or interrupted, ignore this error
                    throw WebServErr::SysCallErrException("accept failed");
                }

                setNonBlocking(connClientFd);
                connMap_[connClientFd] = server->second;
                server->second->addConn(connClientFd);

                registerToEpoll(epollFd_, connClientFd, IN_FLAGS);
            }
            else
            {
                const auto connServer = connMap_.find(events[i].data.fd);
                if (connServer == connMap_.end())
                {
                    LOG_ERROR("Connection not found", "");//TODO: connServer);
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    auto msg = connServer->second->handleDataIn(connServer->first);
                    handleServerMsg(msg, IN, connServer);
                }
                if (events[i].events & EPOLLOUT)
                {
                    auto msg = connServer->second->handleDataOut(connServer->first);
                    handleServerMsg(msg, OUT, connServer);
                }
                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    // TODO connServer->second->handleDataEnd(connServer->first);
                    handleServerMsg({true, -1, IN, -1}, OUT, connServer);
                }
                if (events[i].events & EPOLLRDHUP)
                {
                    // TODO  connServer->second->handleReadEnd(connServer->first);
                    handleServerMsg({true, -1, IN, -1}, IN, connServer);
                }
            }
        }

        timeoutKiller(serverMap_);
    }
}

void WebServ::closeConn(int fd)
{
    if (fd != -1)
    {
        if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1 && errno != ENOENT)
            throw WebServErr::SysCallErrException("epoll_ctl failed");
        close(fd);
    }
    LOG_INFO("Connection closed to: ", fd);
    connMap_.erase(fd);
}

void WebServ::handleServerMsg(const t_msg_from_serv &msg, t_direction direction, const std::unordered_map<int, Server *>::const_iterator &connServer)
{
    if (msg.is_done)
    {
        if (direction == OUT)
            closeConn(connServer->first);
        else if (direction == IN) {}
        else
            throw std::runtime_error("Invalid direction in handleServerMsg");
    }

    if (msg.fd_to_register != -1)
        registerToEpoll(epollFd_, msg.fd_to_register, (msg.direction == IN) ? IN_FLAGS : OUT_FLAGS);
    if (msg.fd_to_unregister != -1)
        closeConn(msg.fd_to_unregister);
}

WebServ::~WebServ()
{
    LOG_INFO("Closing down server.", "");
    for (const auto &server : serverMap_)
    {
        LOG_INFO("Closing Server: ", server.first);
        close(server.first);
    }
    for (const auto &conn : connMap_)
    {
        LOG_INFO("Closing Connection: ", conn.first);
        close(conn.first);
    }
    if (epollFd_ != -1)
        close(epollFd_);
}
