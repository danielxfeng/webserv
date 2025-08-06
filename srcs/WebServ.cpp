#include "WebServ.hpp"

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

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1)
        throw WebServErr::SysCallErrException("epoll_ctl failed");
}

void timeoutKiller(const std::unordered_map<unsigned int, Server *> &serverMap)
{
    for (const auto &server : serverMap)
        server.second->timeoutKiller();
}

WebServ::WebServ()
{
    // TODO:
    // Parse from config file.
    // Add servers to serverVec_.

    // Initialize epoll.
    epollFd_ = epoll_create(0);
    if (epollFd_ == -1)
        throw WebServErr::SysCallErrException("epoll_create failed");
}

void WebServ::eventLoop()
{
    struct epoll_event events[config_.max_poll_events()];

    for (auto it = serverVec_.begin(); it != serverVec_.end(); ++it)
    {
        int serverFd = listenToPort(it->port());
        registerToEpoll(epollFd_, serverFd, EPOLLIN);
        serverMap_[serverFd] = &(*it);
    }

    while (true)
    {
        int numEvents = epoll_wait(epollFd_, events, config_.max_poll_events(), config_.max_poll_timeout());
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

                registerToEpoll(epollFd_, connClientFd, EPOLLIN);
            }
            else
            {
                const auto connServer = connMap_.find(events[i].data.fd);
                if (connServer == connMap_.end())
                {
                    // TODO: Log error: connection not found
                    continue;
                }

                if (events[i].events & EPOLLIN)
                {
                    auto msg = connServer->second->handleRequest(connServer->first);
                    handleServerMsg(msg, IN, connServer);
                }
                if (events[i].events & EPOLLOUT)
                {
                    auto msg = connServer->second->sendResponse(connServer->first);
                    handleServerMsg(msg, OUT, connServer);
                }
                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    connServer->second->closeConn(connServer->first);
                    handleServerMsg({true, -1, IN, -1}, OUT, connServer);
                }
                if (events[i].events & EPOLLRDHUP)
                {
                    connServer->second->closeRead(connServer->first);
                    handleServerMsg({true, -1, IN, -1}, IN, connServer);
                }
            }
        }

        timeoutKiller(serverMap_);
    }
}

void WebServ::closeConn(int fd)
{
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1 && errno != ENOENT)
        throw WebServErr::SysCallErrException("epoll_ctl failed");
    close(fd);
    connMap_.erase(fd);
}

void WebServ::handleServerMsg(const t_msg_from_serv &msg, t_direction direction, const std::unordered_map<unsigned int, Server *>::const_iterator &connServer)
{
    if (msg.is_done)
    {
        if (direction == IN)
            shutdown(connServer->first, SHUT_RD);
        else if (direction == OUT)
            closeConn(connServer->first);
        else
            throw std::runtime_error("Invalid direction in handleServerMsg");
    }

    if (msg.fd_to_register != -1)
        registerToEpoll(epollFd_, msg.fd_to_register, (msg.direction == IN) ? EPOLLIN : EPOLLOUT);
    if (msg.fd_to_unregister != -1)
        closeConn(msg.fd_to_unregister);
}

WebServ::~WebServ()
{
    for (const auto &server : serverMap_)
    {
        close(server.first);
    }
    for (const auto &conn : connMap_)
    {
        close(conn.first);
    }
    close(epollFd_);
}
