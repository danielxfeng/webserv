#pragma once

#include <unordered_map>
#include <string>
#include "TinyJson.hpp"
#include "TinyJsonSerializable.hpp"
#include "Server.hpp"

constexpr unsigned int MAX_POLL_EVENTS = 1024u;
constexpr unsigned int MAX_POLL_TIMEOUT = 1000u; // in milliseconds
constexpr unsigned int MAX_CONNECTIONS = 1024u;
constexpr unsigned int GLOBAL_REQUEST_TIMEOUT = 5000u; // in milliseconds
constexpr unsigned int MAX_REQUEST_SIZE = 1048576u;    // 1 MB
constexpr unsigned int MAX_HEADERS_SIZE = 8192u;       // 8 KB

typedef struct s_server_config
{
    std::string server_name;                                          // Name of the server
    std::string root;                                                 // Root directory for the server
    std::string index;                                                // Default index file
    std::string error_page;                                           // Custom error page
    unsigned int port;                                                // Port number for the server
    unsigned int max_request_timeout;                                 // Maximum request timeout in milliseconds
    unsigned int max_request_size;                                    // Maximum size of a request in bytes
    unsigned int max_headers_size;                                    // Maximum size of headers in bytes
    bool is_cgi;                                                      // Is this an CGI server?
    std::unordered_map<std::string, std::vector<t_method>> locations; // Locations : methods
    std::unordered_map<std::string, std::string> cgi_paths;           // CGI paths for different extensions
} t_server_config;

typedef struct s_global_config
{
    unsigned int max_poll_events;                                  // Maximum number of events to poll
    unsigned int max_poll_timeout;                                 // Maximum timeout for polling events in milliseconds
    unsigned int max_connections;                                  // Maximum size of a request in bytes
    unsigned int global_request_timeout;                           // Global request timeout in milliseconds
    unsigned int max_request_size;                                 // Maximum size of a request in bytes
    unsigned int max_headers_size;                                 // Maximum size of headers in bytes
    std::unordered_map<std::string, t_server_config> server_names; // Server names and their corresponding configurations
} t_global_config;

class Config : public TinyJsonSerializable
{
private:
    t_global_config global_config_;

public:
    Config() = default;
    Config(const Config &other) = default;
    Config &operator=(const Config &other) = default;
    ~Config() = default;

    std::string toJson() const override;

    void fromJson(const std::string &jsonString) override;
};