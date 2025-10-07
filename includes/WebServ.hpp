#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <csignal>
#include "Server.hpp"
#include "Config.hpp"
#include "WebServErr.hpp"
#include "LogSys.hpp"
#include "EpollHelper.hpp"
#include "RaiiFd.hpp"

void handleSignal(int sig);
extern volatile std::sig_atomic_t stopFlag;
const size_t MAX_CONN = 250;

class WebServ
{
private:
    EpollHelper epoll_;                            // Singleton epoll instance
    t_global_config config_;                       // Config
    std::list<Server> servers_;                    // Vector of servers, the instances.
    std::list<std::shared_ptr<RaiiFd>> fds_;       // List of RAII wrappers for listening sockets
    std::unordered_map<int, Server *> server_map_; // Maps `listen` file descriptors to pointers to Server instances
    std::unordered_map<int, Server *> conn_map_;   // Maps `connections` file descriptors to pointers to Server instances
    void handleServerMsg(const t_msg_from_serv &msg, Server *server);
    void timeoutKiller(const std::unordered_map<int, Server *> &serverMap);

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
    /**
     * @brief Add a file descriptor to the epoll instance.
     */
    void addFdToEpoll(const std::shared_ptr<RaiiFd> fd, Server *server);
};
