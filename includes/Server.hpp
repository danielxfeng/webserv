#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Buffer.hpp"
#include "MethodHandler.hpp"

typedef enum e_direction
{
    IN,
    OUT
} t_direction;

typedef enum e_method
{
    GET,
    POST,
    DELETE,
    CGI,
    UNKNOWN,
} t_method;

typedef enum e_status
{
    HEADER_PARSING,
    READING,
    WRITING,
    DONE,
    ERROR
} t_status;

typedef enum e_error_codes
{
    NO_ERROR,
    NOT_FOUND,
    BAD_REQUEST,
    INTERNAL_SERVER_ERROR
} t_error_code;

typedef struct s_conn
{
    int socket_fd;
    int inner_fd_in;
    int inner_fd_out;
    std::string path;
    t_method method;
    t_status status;
    time_t start_timestamp;
    time_t last_heartbeat;
    size_t content_length;
    size_t bytes_received;
    Buffer read_buf;
    Buffer write_buf;
    std::unordered_map<std::string, std::string> headers;
} t_conn;

typedef struct s_msg_from_serv
{
    bool is_done;          // Indicates if the operation is complete
    int fd_to_register;    // File descriptor to register for epoll
    t_direction direction; // Direction of the fd_to_register
    int fd_to_unregister;  // File descriptor to unregister from epoll
} t_msg_from_serv;

t_method convertMethod(const std::string &method_str);

// TODO: to be implemented
class Server
{
private:
    unsigned int port_;
    std::string name_;
    unsigned int max_header_size_;
    std::vector<t_conn> conn_vec_;
    std::unordered_map<int, t_conn *> conn_map_;

public:
    unsigned int port() const { return port_; }

    void addConn(int fd) { /* TODO: implement */ }
    void handleDataEnd(int fd) { /* TODO: implement */ }
    void handleReadEnd(int fd) { /* TODO: implement */ }
    t_msg_from_serv handleDataIn(int fd);
    t_msg_from_serv handleDataOut(int fd);
    t_msg_from_serv handleError(t_conn *conn, t_error_code error_code, const std::string &error_message);
    void timeoutKiller() { /* TODO: implement */ }
};
