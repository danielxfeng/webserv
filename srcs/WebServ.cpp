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

void registerToEpoll(int epollFd, int fd, int direction)
{
    struct epoll_event event{};
    event.events = direction;
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

	LOG_INFO("Server started.");
    for (auto it = serverVec_.begin(); it != serverVec_.end(); ++it)
    {
        int serverFd = listenToPort(it->port());
		LOG_INFO("Server listening to port:", serverFd);
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
                bool ifClose = false;
                if (connServer == connMap_.end())
                {
                    LOG_ERROR("Connection not found", connServer);
                    continue;
                }

                if (events[i].events & EPOLLIN)
                    ifClose = connServer->second->handleRequest(connServer->first);
                if (events[i].events & EPOLLOUT)
                {
                    const bool sendResponseClose = connServer->second->sendResponse(connServer->first);
                    ifClose = ifClose || sendResponseClose;
                }
                if (events[i].events & (EPOLLHUP | EPOLLERR))
                {
                    connServer->second->closeConn(connServer->first);
                    ifClose = true;
                }

                if (ifClose)
                    closeConn(connServer->first);
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
	LOG_INFO("Connection closed to: ", fd);
    connMap_.erase(fd);
}

WebServ::~WebServ()
{
	LOG_INFO("Closing down server.");
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
    close(epollFd_);
}
