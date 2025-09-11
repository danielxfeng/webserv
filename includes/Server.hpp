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

    /**
     * @brief Helper function to close a connection and clean up resources.
     * @details Removes all the fds from conns, will not remove the conn from conns_ list.
     */
    

    t_msg_from_serv handleDataInFromSocket(int fd, t_conn *conn);
    t_msg_from_serv handleDataInFromSocketParsingHeader(int fd, t_conn *conn, bool is_eof);
    t_msg_from_serv handleDataInFromSocketReadingBody(int fd, t_conn *conn, bool is_eof);
    t_msg_from_serv handleDataInFromInternal(int fd, t_conn *conn);
    t_msg_from_serv Server::handleDataOutToSocketDone (int fd, t_conn *conn);
    t_msg_from_serv handleDataOutToSocket(int fd, t_conn *conn);
    t_msg_from_serv handleDataOutToInternal(int fd, t_conn *conn);

    t_msg_from_serv closeConn(t_conn *conn);
    t_msg_from_serv resetConnMap(t_conn *conn);

    t_msg_from_serv req_header_parsing_handler(int fd, t_conn *conn);
    t_msg_from_serv req_header_processing_handler(int fd, t_conn *conn);
    t_msg_from_serv req_body_processing_in_handler(int fd, t_conn *conn);
    t_msg_from_serv req_body_processing_out_handler(int fd, t_conn *conn);
    t_msg_from_serv res_header_processing_handler(int fd, t_conn *conn);
    t_msg_from_serv response_in_handler(int fd, t_conn *conn);
    t_msg_from_serv response_out_handler(int fd, t_conn *conn);
    t_msg_from_serv done_handler(int fd, t_conn *conn);

    /**
     * @brief Handler for terminated connections.
     * @details 
     * Removes the conn from all the maps, and lists, fds are closed by RaiiFd.
     * Be called in scheduler when event_type is ERROR_EVENT, or when a connection should be terminated in FSM.
     * Will not send any response to the client. 
     */
    t_msg_from_serv terminated_handler(int fd, t_conn *conn);

public:
    Server() = delete;
    Server(EpollHelper &epoll, const t_server_config &config);
    Server(const Server &) = default;
    Server(Server &&) = default;
    Server &operator=(const Server &) = delete;
    ~Server() = default;

    const t_server_config &getConfig() const;

    void addConn(int fd);
    t_msg_from_serv handleDataEnd(int fd);

    t_msg_from_serv handleDataIn(int fd);
    t_msg_from_serv handleDataOut(int fd);
    t_msg_from_serv handleError(t_conn *conn, const t_status_error_codes error_code, const std::string &error_message);

    t_msg_from_serv scheduler(int fd, t_event_type event_type);
    t_msg_from_serv timeoutKiller();
};
