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

class WebServ
{
private:
    int epollFd_;                                          // Singleton epoll file descriptor
    Config config_;                                        // Configuration instance
    std::vector<Server> serverVec_;                        // Vector of servers, the instances.
    std::unordered_map<unsigned int, Server *> serverMap_; // Maps `listen` file descriptors to pointers to Server instances
    std::unordered_map<unsigned int, Server *> connMap_;   // Maps `connections` file descriptors to pointers to Server instances

public:
    WebServ(const WebServ &other) = delete;
    WebServ &operator=(const WebServ &other) = delete;
    /**
     * @brief Constructor for WebServ.
     * @details 1 Load the config, 2 Create servers, 3 Create epoll.
     */
    WebServ();
    /**
     * @brief Destructor for WebServ.
     * @details Close all connections, close file descriptors, and close epoll.
     */
    ~WebServ();
    /**
     * @brief Main event loop for the web server.
     */
    void eventLoop();
    /**
     * @brief Closes a connection.
     */
    void closeConn(int fd);
};
