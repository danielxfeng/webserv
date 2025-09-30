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
#include "utils.hpp"
#include "ErrorResponse.hpp"

class Config;

/**
 * @brief Server class to manage connections.
 * @details
 * Implemented by a finite state machine (FSM).
 * The scheduler method routes events to state handlers.
 * Each handler processes events and may transition the state.
 */
class Server
{
private:
    EpollHelper &epoll_;                                            // Reference to the epoll helper
    std::vector<t_server_config> configs_;                          // List of server configurations
    std::list<t_conn> conns_;                                       // List of active connections
    std::vector<std::string> cookies_;                              // List of cookies for session management
    std::unordered_map<int, t_conn *> conn_map_;                    // Map of fds(in epoll) to connections
    std::unordered_map<int, std::shared_ptr<RaiiFd>> inner_fd_map_; // Map of internal fds to RaiiFd objects

    //
    // Helper functions
    //

    t_msg_from_serv closeConn(t_conn *conn);
    t_msg_from_serv resetConnMap(t_conn *conn);

    //
    // Finite State Machine (FSM) Handlers
    //

    /**
     * @brief Handler for parsing request headers.
     */
    t_msg_from_serv reqHeaderParsingHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for processing request headers.
     */
    t_msg_from_serv reqHeaderProcessingHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for processing request body.
     */
    t_msg_from_serv reqBodyProcessingInHandler(int fd, t_conn *conn, bool is_initial = false);

    /**
     * @brief Handler for processing request body (for CGI).
     */
    t_msg_from_serv reqBodyProcessingOutHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for processing response headers.
     */
    t_msg_from_serv resheaderProcessingHandler(t_conn *conn);

    /**
     * @brief Handler for receiving response data from pipe (for CGI).
     */
    t_msg_from_serv responseInHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for sending response data to the client.
     */
    t_msg_from_serv responseOutHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for completed connections.
     */
    t_msg_from_serv doneHandler(int fd, t_conn *conn);

    /**
     * @brief Handler for terminated connections.
     */
    t_msg_from_serv terminatedHandler(int fd, t_conn *conn);

public:
    Server() = delete;
    Server(EpollHelper &epoll, const std::vector<t_server_config> &configs);
    Server(const Server &) = default;
    Server(Server &&) = default;
    Server &operator=(const Server &) = delete;
    ~Server() = default;

    /**
     * @brief Returns the server configurations.
     */
    const std::vector<t_server_config> &getConfigs() const;

    /**
     * @brief Returns the server configuration at the given index.
     */
    const t_server_config &getConfig(size_t idx) const;

    /**
     * @brief Checks for timed-out connections and closes them.
     */
    t_msg_from_serv timeoutKiller();

    /**
     * @brief Creates and adds a new connection for the given fd.
     */
    void addConn(int fd);

    /**
     * @brief A fsm scheduler to handle events for connections.
     */
    t_msg_from_serv scheduler(int fd, t_event_type event_type);
};
