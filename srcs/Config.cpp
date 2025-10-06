#include "../includes/Config.hpp"


const std::string randomServerName()
{
    static int counter = 1; // Static counter to ensure unique names
    return "server_" + std::to_string(counter++);
}

t_status_error_codes stringToErrCode(std::string str)
{
    if (str == "301")
        return ERR_301_REDIRECT;
    if (str == "400")
        return ERR_400_BAD_REQUEST;
    if (str == "401")
        return ERR_401_UNAUTHORIZED;
    if (str == "403")
        return ERR_403_FORBIDDEN;
    if (str == "404")
        return ERR_404_NOT_FOUND;
    if (str == "405")
        return ERR_405_METHOD_NOT_ALLOWED;
    if (str == "409")
        return ERR_409_CONFLICT;
    if (str == "500")
        return ERR_500_INTERNAL_SERVER_ERROR;
    throw std::invalid_argument("invalid error code");
}

const t_global_config &Config::getGlobalConfig() const
{
    return global_config_;
}

t_global_config Config::fetchGlobalConfig()
{
    return global_config_;
}

void Config::fromJson(const std::string &json_string)
{
    const JsonValue json_value = TinyJson::parse(json_string);
    const JsonObject json_obj = TinyJson::as<JsonObject>(json_value);

    global_config_.max_poll_events = TinyJson::as<unsigned int>(*json_obj.at("max_poll_events"), MAX_POLL_EVENTS);
    global_config_.max_poll_timeout = TinyJson::as<unsigned int>(*json_obj.at("max_poll_timeout"), MAX_POLL_TIMEOUT);
    global_config_.global_request_timeout = TinyJson::as<unsigned int>(*json_obj.at("global_request_timeout"), GLOBAL_REQUEST_TIMEOUT);
    global_config_.max_request_size = TinyJson::as<unsigned int>(*json_obj.at("max_request_size"), MAX_REQUEST_SIZE);
    global_config_.max_headers_size = TinyJson::as<unsigned int>(*json_obj.at("max_headers_size"), MAX_HEADERS_SIZE);
    global_config_.max_heartbeat_timeout = TinyJson::as<unsigned int>(*json_obj.at("max_heartbeat_timeout"), GLOBAL_HEARTBEAT_TIMEOUT);

    const JsonArray server_array = TinyJson::as<JsonArray>(*json_obj.at("servers"));
    for (const auto &server_value_ptr : server_array)
    {
        const JsonObject server_obj = TinyJson::as<JsonObject>(*server_value_ptr);
        t_server_config server_config;
        
        server_config.server_name = server_obj.contains("server_name") ? toLower(TinyJson::as<std::string>(*server_obj.at("server_name"))) : randomServerName();
        
        server_config.port = TinyJson::as<unsigned int>(*server_obj.at("port"));


        const unsigned int max_request_timeout = server_obj.contains("max_request_timeout") ? TinyJson::as<unsigned int>(*server_obj.at("max_request_timeout")) : global_config_.global_request_timeout;
        server_config.max_request_timeout = std::min(max_request_timeout, global_config_.global_request_timeout);

        const unsigned int max_heartbeat_timeout = server_obj.contains("max_heartbeat_timeout") ? TinyJson::as<unsigned int>(*server_obj.at("max_heartbeat_timeout")) : global_config_.max_heartbeat_timeout;
        server_config.max_heartbeat_timeout = std::min(max_heartbeat_timeout, global_config_.max_heartbeat_timeout);

        const unsigned int max_request_size = server_obj.contains("max_request_size") ? TinyJson::as<unsigned int>(*server_obj.at("max_request_size")) : global_config_.max_request_size;
        server_config.max_request_size = std::min(max_request_size, global_config_.max_request_size);

        const unsigned int max_headers_size = server_obj.contains("max_headers_size") ? TinyJson::as<unsigned int>(*server_obj.at("max_headers_size")) : global_config_.max_headers_size;
        server_config.max_headers_size = std::min(max_headers_size, global_config_.max_headers_size);

        server_config.is_cgi = server_obj.contains("is_cgi") ? TinyJson::as<bool>(*server_obj.at("is_cgi")) : false;

        if (server_config.is_cgi)
        {
            JsonObject cgi_obj = TinyJson::as<JsonObject>(*server_obj.at("cgi_config"));
            for (const auto &cgi_pair : cgi_obj)
                server_config.cgi_paths[cgi_pair.first] = TinyJson::as<std::string>(*cgi_pair.second);
        }
        else
        {
            JsonObject locations_obj = TinyJson::as<JsonObject>(*server_obj.at("locations"));
            for (const auto &location_pair : locations_obj)
            {
                const std::string location_path = location_pair.first;
                const JsonObject location_obj = TinyJson::as<JsonObject>(*location_pair.second);
                t_location_config location_config;
                location_config.root = TinyJson::as<std::string>(*location_obj.at("root"));
                location_config.index = location_obj.contains("index") ? TinyJson::as<std::string>(*location_obj.at("index")) : "";

                JsonArray methods_array = TinyJson::as<JsonArray>(*location_obj.at("methods"));
                for (const auto &method_value_ptr : methods_array)
                {
                    t_method method = convertMethod(TinyJson::as<std::string>(*method_value_ptr));
                    if (method == CGI || method == UNKNOWN)
                        throw std::invalid_argument("invalid method in locations: " + location_path);
                    location_config.methods.push_back(method);
                }
                if (std::find(location_config.methods.begin(), location_config.methods.end(), REDIRECT) != location_config.methods.end()
                    && location_config.methods.size() > 1)
                    throw std::invalid_argument("invalid method combination in location: " + location_path);
                server_config.locations[location_path] = location_config;
            }
        }

        if (server_obj.contains("error_pages"))
        {
            const auto err_pages = TinyJson::as<JsonObject>(*server_obj.at("error_pages"));
            for (const auto& [key, val] : err_pages) {
                server_config.err_pages[stringToErrCode(TinyJson::as<std::string>(key))] =
                    TinyJson::as<std::string>(*val);
            }
        }

        global_config_.servers.push_back(server_config);
    }

    //validate the porst and the server name
    for (auto it = global_config_.servers.begin(); it != global_config_.servers.end(); ++it) {
        for (auto it2 = std::next(it); it2 != global_config_.servers.end(); ++it2) {
            if (it->port == it2->port && it->server_name == it2->server_name)
                throw std::invalid_argument("Duplicated port and servername");
        }
}

}

t_global_config Config::parseConfigFromFile(const std::string &conf_file_path)
{
    std::ifstream conf_file(conf_file_path);
    if (!conf_file.is_open())
        throw std::runtime_error("Failed to open config file: " + conf_file_path);

    const std::string json{
        std::istreambuf_iterator<char>(conf_file),
        std::istreambuf_iterator<char>()};

    Config config;

    config.fromJson(json);
    return config.fetchGlobalConfig();
}
