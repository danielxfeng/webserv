#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "Server.hpp"
#include "Config.hpp"
#include "WebServErr.hpp"

inline constexpr uint32_t IN_FLAGS = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
inline constexpr uint32_t OUT_FLAGS = EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR;

class WebServ
{
private:
    int epollFd_;                                 // Singleton epoll file descriptor
    const t_global_config config_;                // Config
    std::vector<Server> serverVec_;               // Vector of servers, the instances.
    std::unordered_map<int, Server *> serverMap_; // Maps `listen` file descriptors to pointers to Server instances
    std::unordered_map<int, Server *> connMap_;   // Maps `connections` file descriptors to pointers to Server instances

    void closeConn(int fd);
    void handleServerMsg(const t_msg_from_serv &msg, t_direction direction, const std::unordered_map<int, Server *>::const_iterator &connServer);

public:
    WebServ(const WebServ &other) = delete;
    WebServ &operator=(const WebServ &other) = delete;
    WebServ() = delete;
    /**
     * @brief Constructor for WebServ.
     * @details 1 Load the config, 2 Create servers, 3 Create epoll.
     */
    WebServ(const std::string &conf_file);
    /**
     * @brief Destructor for WebServ.
     * @details Close all connections, close file descriptors, and close epoll.
     */
    ~WebServ();
    /**
     * @brief Main event loop for the web server.
     */
    void eventLoop();
};
