#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <ctime>
#include <memory>

/**
 * @brief Enumeration of HTTP status error codes.
 */
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
    ERR_500_INTERNAL_SERVER_ERROR = 500,
    ERR_501_NOT_IMPLEMENTED = 501
} t_status_error_codes;

constexpr unsigned int MAX_POLL_EVENTS = 1024u;
constexpr unsigned int MAX_POLL_TIMEOUT = 1000u;                    // in milliseconds
constexpr unsigned int GLOBAL_REQUEST_TIMEOUT = 10000u;             // in milliseconds
constexpr unsigned int GLOBAL_HEARTBEAT_TIMEOUT = 5000u;            // 5 seconds
constexpr size_t MAX_REQUEST_SIZE = 2048 * 1024 * 1024u;            // 2 GB
constexpr unsigned int MAX_HEADERS_SIZE = 8192u;                    // 8 KB

class HttpRequests;
class HttpResponse;
class Buffer;
class RaiiFd;

typedef struct s_FormData
{
    std::string name_;
    std::string type_;
    std::string content_;
} t_FormData;

typedef struct s_file
{
    std::shared_ptr<RaiiFd> FD_handler_IN;
    std::shared_ptr<RaiiFd> FD_handler_OUT;
    size_t expectedSize;
    size_t fileSize;
    bool isDynamic;
    std::string dynamicPage;
    std::string postFilename;
    pid_t pid;
} t_file;

/**
 * @brief Enumeration of buffer error codes.
 */
typedef enum e_buff_error_code
{
    EOF_REACHED = 0,
    RW_ERROR = -1,
    BUFFER_FULL = -2,
    BUFFER_EMPTY = -3,
    CHUNKED_ERR = -4
} t_buff_error_code;

/**
 * @brief Enumeration of HTTP methods.
 */
typedef enum e_method
{
    GET,
    POST,
    DELETE,
    CGI,
    REDIRECT,
    UNKNOWN,
} t_method;

typedef enum e_event_type
{
    READ_EVENT,
    WRITE_EVENT,
    ERROR_EVENT
} t_event_type;

typedef enum e_direction
{
    IN,
    OUT
} t_direction;

/**
 * @brief Enumeration of connection statuses.
 */
typedef enum e_status
{
    REQ_HEADER_PARSING,    // Stage for parsing HTTP headers
    REQ_HEADER_PROCESSING, // Stage for processing HTTP headers
    REQ_BODY_PROCESSING,   // Stage for processing HTTP body
    RES_HEADER_PROCESSING, // Stage for preparing HTTP response headers
    RESPONSE,              // Stage for writing HTTP response
    DONE,                  // Stage indicating the connection is done
    TERMINATED             // Stage indicating the connection is terminated
} t_status;

/**
 * @brief Structure representing a client connection.
 */
typedef struct s_conn
{
    int socket_fd;                          // Client socket file descriptor
    int inner_fd_in;                        // Internal file descriptor for reading request body
    int inner_fd_out;                       // Internal file descriptor for writing response body
    int config_idx;                         // Index of the server configuration used
    bool is_cgi;                            // Is this connection handling a CGI request?
    bool cgi_header_ready;                  // Is the CGI response header ready
    t_status status;                        // Current status of the connection
    t_status_error_codes error_code;        // Error code if any error occurs
    time_t start_timestamp;                 // Timestamp when the connection was established
    time_t last_heartbeat;                  // Timestamp of the last heartbeat received
    size_t content_length;                  // Expected content length of the request body
    size_t bytes_received;                  // Read from socket
    size_t output_length;                   // Expected output length of the response body
    size_t bytes_sent;                      // Sent to socket
    t_file res;                             // File/resource associated with the response
    std::unique_ptr<Buffer> read_buf;       // Buffer for reading data
    std::unique_ptr<Buffer> write_buf;      // Buffer for writing data
    std::shared_ptr<HttpRequests> request;  // Parsed HTTP request
    std::shared_ptr<HttpResponse> response; // HTTP response generator
    std::string error_message;              // Error  message
} t_conn;

/**
 * @brief Structure representing a message from the server to the main event loop.
 */
typedef struct s_msg_from_serv
{
    std::vector<std::shared_ptr<RaiiFd>> fds_to_register;
    std::vector<int> fds_to_unregister;
} t_msg_from_serv;

typedef struct s_location_config
{
    std::vector<t_method> methods; // Allowed methods for this location
    std::string root;              // Root directory for this location
    std::string index;             // Default index file for this location
} t_location_config;

typedef struct s_cgi_config
{
    std::string root;        // Default root path for this CGI
    std::string interpreter; // Interpreter path for this CGI
} t_cgi_config;

/**
 * @brief Structure representing the configuration of a server.
 */
typedef struct s_server_config
{
    std::string server_name;                                         // Name of the server
    unsigned int port;                                               // Port number for the server
    unsigned int max_request_timeout;                                // Maximum request timeout in milliseconds
    unsigned int max_heartbeat_timeout;                              // Maximum heartbeat timeout in milliseconds
    unsigned int max_request_size;                                   // Maximum size of a request in bytes
    unsigned int max_headers_size;                                   // Maximum size of headers in bytes
    bool is_cgi;                                                     // Is this an CGI server?
    std::unordered_map<std::string, t_location_config> locations;    // Locations : methods
    std::unordered_map<std::string, t_cgi_config> cgi_paths;         // CGI paths for different extensions
    std::unordered_map<t_status_error_codes, std::string> err_pages; // Error Page paths
} t_server_config;

/**
 * @brief Structure representing the global configuration of the server.
 */
typedef struct s_global_config
{
    unsigned int max_poll_events;         // Maximum number of events to poll
    unsigned int max_poll_timeout;        // Maximum timeout for polling events in milliseconds
    unsigned int global_request_timeout;  // Global request timeout in milliseconds
    unsigned int max_heartbeat_timeout;   // Global heartbeat timeout in milliseconds
    unsigned int max_request_size;        // Maximum size of a request in bytes
    unsigned int max_headers_size;        // Maximum size of headers in bytes
    std::vector<t_server_config> servers; // Server names and their corresponding configurations
} t_global_config;

t_method convertMethod(const std::string &method_str);
