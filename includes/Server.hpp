#pragma once

#include <string>
#include <list>
#include <vector>
#include <unordered_map>
#include "Buffer.hpp"
#include "MethodHandler.hpp"
#include "Config.hpp"
#include "SharedTypes.hpp"
#include "HttpRequests.hpp"
#include "HttpResponse.hpp"
#include "MethodHandler.hpp"
#include "RaiiFd.hpp"
#include "LogSys.hpp"
#include "WebServErr.hpp"

class Config;

class Server
{
private:
    EpollHelper &epoll_;            // Reference to the epoll helper
    const t_server_config &config_; // Server configuration
    std::list<t_conn> conns_;
    std::vector<std::string> cookies_;
    std::unordered_map<int, t_conn *> conn_map_;

    t_msg_from_serv closeConn(t_conn *conn);
    t_msg_from_serv resetConnMap(t_conn *conn);

    t_msg_from_serv reqHeaderParsingHandler(int fd, t_conn *conn);
    t_msg_from_serv reqHeaderProcessingHandler(int fd, t_conn *conn);
    t_msg_from_serv reqBodyProcessingInHandler(int fd, t_conn *conn);
    t_msg_from_serv reqBodyProcessingOutHandler(int fd, t_conn *conn);
    t_msg_from_serv resheaderProcessingHandler(t_conn *conn);
    t_msg_from_serv responseInHandler(int fd, t_conn *conn);
    t_msg_from_serv responseOutHandler(int fd, t_conn *conn);
    t_msg_from_serv doneHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for terminated connections.
     * @details
     * Removes the conn from all the maps, and lists, fds are closed by RaiiFd.
     * Be called in scheduler when event_type is ERROR_EVENT, or when a connection should be terminated in FSM.
     * Will not send any response to the client.
     */
    t_msg_from_serv terminatedHandler(int fd, t_conn *conn);

public:
    Server() = delete;
    Server(EpollHelper &epoll, const t_server_config &config);
    Server(const Server &) = default;
    Server(const Server &&) = default;
    Server(Server &&) = default;
    Server &operator=(const Server &) = delete;
    ~Server() = default;

    const t_server_config &getConfig() const;

    void addConn(int fd);
    t_msg_from_serv scheduler(int fd, t_event_type event_type);
    t_msg_from_serv timeoutKiller();
};
