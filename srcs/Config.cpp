#include "../includes/Config.hpp"

// TODO: remove this after we can include server.hpp
t_method convertMethod(const std::string &method_str)
{
    std::string upper_method = method_str;
    std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(), ::toupper);
    if (upper_method == "GET")
        return GET;
    else if (upper_method == "POST")
        return POST;
    else if (upper_method == "DELETE")
        return DELETE;
    else if (upper_method == "CGI")
        return CGI;
    else
        return UNKNOWN;
}

t_global_config &Config::getGlobalConfig()
{
    return global_config_;
}

void Config::fromJson(const std::string &json_string)
{
    const JsonValue json_value = TinyJson::parse(json_string);
    const JsonObject &json_obj = std::get<JsonObject>(json_value);
    global_config_.max_poll_events = TinyJson::as<unsigned int>(json_obj.at("max_poll_events"), MAX_POLL_EVENTS);
    global_config_.max_poll_timeout = TinyJson::as<unsigned int>(json_obj.at("max_poll_timeout"), MAX_POLL_TIMEOUT);
    global_config_.max_connections = TinyJson::as<unsigned int>(json_obj.at("max_connections"), MAX_CONNECTIONS);
    global_config_.global_request_timeout = TinyJson::as<unsigned int>(json_obj.at("global_request_timeout"), GLOBAL_REQUEST_TIMEOUT);
    global_config_.max_request_size = TinyJson::as<unsigned int>(json_obj.at("max_request_size"), MAX_REQUEST_SIZE);
    global_config_.max_headers_size = TinyJson::as<unsigned int>(json_obj.at("max_headers_size"), MAX_HEADERS_SIZE);

    const JsonArray &server_array = std::get<JsonArray>(json_obj.at("servers"));
    for (const JsonValue &server_value : server_array)
    {
        const JsonObject &server_obj = std::get<JsonObject>(server_value);
        t_server_config server_config;
        server_config.server_name = TinyJson::as<std::string>(server_obj.at("server_name"));
        server_config.root = server_obj.contains("root") ? TinyJson::as<std::string>(server_obj.at("root")) : "";
        server_config.index = server_obj.contains("index") ? TinyJson::as<std::string>(server_obj.at("index")) : "";
        server_config.port = TinyJson::as<unsigned int>(server_obj.at("port"));

        const unsigned int max_request_timeout = server_obj.contains("max_request_timeout") ? TinyJson::as<unsigned int>(server_obj.at("max_request_timeout")) : global_config_.global_request_timeout;
        server_config.max_request_timeout = std::min(max_request_timeout, global_config_.global_request_timeout);

        const unsigned int max_request_size = server_obj.contains("max_request_size") ? TinyJson::as<unsigned int>(server_obj.at("max_request_size")) : global_config_.max_request_size;
        server_config.max_request_size = std::min(max_request_size, global_config_.max_request_size);

        const unsigned int max_headers_size = server_obj.contains("max_headers_size") ? TinyJson::as<unsigned int>(server_obj.at("max_headers_size")) : global_config_.max_headers_size;
        server_config.max_headers_size = std::min(max_headers_size, global_config_.max_headers_size);

        server_config.is_cgi = server_obj.contains("is_cgi") ? TinyJson::as<bool>(server_obj.at("is_cgi")) : false;

        if (server_config.is_cgi)
        {
            const JsonObject &cgi_obj = std::get<JsonObject>(server_obj.at("cgi_config"));
            for (const auto &cgi_pair : cgi_obj)
                server_config.cgi_paths[cgi_pair.first] = TinyJson::as<std::string>(cgi_pair.second);
        }
        else
        {
            const JsonObject &locations_obj = std::get<JsonObject>(server_obj.at("locations"));
            for (const auto &location_pair : locations_obj)
            {
                const std::string &location_path = location_pair.first;
                const JsonArray &methods_array = std::get<JsonArray>(location_pair.second);
                for (const JsonValue &method_value : methods_array)
                {
                    t_method method = convertMethod(TinyJson::as<std::string>(method_value));
                    if (method == CGI || method == UNKNOWN)
                        throw std::invalid_argument("invalid method in locations: " + location_path);
                    server_config.locations[location_path].push_back(method);
                }
            }
        }
        if (global_config_.servers.contains(server_config.server_name))
            throw std::invalid_argument("Duplicate server name: " + server_config.server_name);

        global_config_.servers[server_config.server_name] = server_config;
    }
}
