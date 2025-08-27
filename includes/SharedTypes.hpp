#pragma once

#include <string>
#include <unordered_map>
#include <ctime>

#include "Buffer.hpp"


constexpr unsigned int MAX_POLL_EVENTS = 1024u;
constexpr unsigned int MAX_POLL_TIMEOUT = 1000u; // in milliseconds
constexpr unsigned int GLOBAL_REQUEST_TIMEOUT = 5000u; // in milliseconds
constexpr unsigned int MAX_REQUEST_SIZE = 1048576u;    // 1 MB
constexpr unsigned int MAX_HEADERS_SIZE = 8192u;       // 8 KB


typedef enum e_method
{
    GET,
    POST,
    DELETE,
    CGI,
    UNKNOWN,
} t_method;

typedef enum e_direction
{
    IN,
    OUT
} t_direction;

typedef enum e_status
{
    HEADER_PARSING,
    READING,
    WRITING,
    DONE,
    SRV_ERROR
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


typedef struct s_location_config
{
    std::vector<t_method> methods; // Allowed methods for this location
    std::string root;              // Root directory for this location
    std::string index;             // Default index file for this location
} t_location_config;

typedef struct s_server_config
{
    std::string server_name;                                      // Name of the server
    unsigned int port;                                            // Port number for the server
    unsigned int max_request_timeout;                             // Maximum request timeout in milliseconds
    unsigned int max_request_size;                                // Maximum size of a request in bytes
    unsigned int max_headers_size;                                // Maximum size of headers in bytes
    bool is_cgi;                                                  // Is this an CGI server?
    std::unordered_map<std::string, t_location_config> locations; // Locations : methods
    std::unordered_map<std::string, std::string> cgi_paths;       // CGI paths for different extensions
} t_server_config;

typedef struct s_global_config
{
    unsigned int max_poll_events;                             // Maximum number of events to poll
    unsigned int max_poll_timeout;                            // Maximum timeout for polling events in milliseconds
    unsigned int global_request_timeout;                      // Global request timeout in milliseconds
    unsigned int max_request_size;                            // Maximum size of a request in bytes
    unsigned int max_headers_size;                            // Maximum size of headers in bytes
    std::unordered_map<std::string, t_server_config> servers; // Server names and their corresponding configurations
} t_global_config;

t_method convertMethod(const std::string &method_str);
