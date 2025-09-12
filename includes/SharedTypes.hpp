#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <ctime>
#include <memory>

typedef enum e_status_error_codes
{
    ERR_NO_ERROR = 0,
    ERR_301_REDIRECT = 301,
    ERR_400_BAD_REQUEST = 400,
    ERR_401_UNAUTHORIZED = 401,
    ERR_403_FORBIDDEN = 403,
    ERR_404_NOT_FOUND = 404,
    ERR_405_METHOD_NOT_ALLOWED = 405,
    ERR_409_CONFLICT = 409,
    ERR_500_INTERNAL_SERVER_ERROR = 500
} t_status_error_codes;

constexpr unsigned int MAX_POLL_EVENTS = 1024u;
constexpr unsigned int MAX_POLL_TIMEOUT = 1000u;       // in milliseconds
constexpr unsigned int GLOBAL_REQUEST_TIMEOUT = 5000u; // in milliseconds
constexpr unsigned int MAX_REQUEST_SIZE = 1048576u;    // 1 MB
constexpr unsigned int MAX_HEADERS_SIZE = 8192u;       // 8 KB

class HttpRequests;
class HttpResponse;
class Buffer;

typedef struct s_FormData
{
    std::string name_;
    std::string type_;
    std::string content_;
} t_FormData;

typedef struct s_file
{
    int fd;
    size_t expectedSize;
    size_t fileSize;
    bool isDynamic;
    std::string dynamicPage;
} t_file;

typedef enum e_buff_error_code
{
    EOF_REACHED = 0,
    BUFFER_ERROR = -1,
    BUFFER_FULL = -2,
    BUFFER_EMPTY = -3,
    CHUNKED_ERR = -4
} t_buff_error_code;

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

typedef struct s_conn
{
    int socket_fd;
    int inner_fd_in;
    int inner_fd_out;
    t_status status;
    time_t start_timestamp;
    time_t last_heartbeat;
    size_t content_length;
    size_t bytes_received;
    std::unique_ptr<Buffer> read_buf;
    std::unique_ptr<Buffer> write_buf;
    std::shared_ptr<HttpRequests> request;
    std::shared_ptr<HttpResponse> response;
} t_conn;

typedef struct s_msg_from_serv
{
    bool is_done;                                               // Indicates if the operation is complete
    std::vector<std::pair<int, t_direction>> fds_to_register;   // File descriptors to register for epoll with their directions
    std::vector<std::pair<int, t_direction>> fds_to_unregister; // File descriptors to unregister from epoll with their directions
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
