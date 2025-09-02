#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Buffer.hpp"
#include "MethodHandler.hpp"
#include "Config.hpp"
#include "SharedTypes.hpp"
#include "HttpRequests.hpp"
#include "HttpResponse.hpp"
#include "MethodHandler.hpp"

class Config;

class Server
{
private:
    const t_server_config &config_; // Server configuration
    std::vector<t_conn> conn_vec_;
    std::unordered_map<int, t_conn *> conn_map_;

public:
    Server() = delete;
    Server(const t_server_config &config);
    Server(const Server &) = default;
    Server(Server &&) = default;
    Server &operator=(const Server &) = delete;
    ~Server() = default;

    const t_server_config &getConfig() const;

    void addConn(int fd);
    void handleDataEnd(int fd);
    void handleReadEnd(int fd);
    t_msg_from_serv handleDataIn(int fd);
    t_msg_from_serv handleDataOut(int fd);
    t_msg_from_serv handleError(t_conn *conn, const std::string &error_message);
    void timeoutKiller();
};
