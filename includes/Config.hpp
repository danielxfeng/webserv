#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "../includes/TinyJson.hpp"
#include "../includes/TinyJsonSerializable.hpp"
#include "../includes/SharedTypes.hpp"

class Server;
class Config : public TinyJsonSerializable
{
private:
    t_global_config global_config_;

public:
    Config() = default;
    Config(const Config &other) = default;
    Config &operator=(const Config &other) = default;
    ~Config() = default;

    void fromJson(const std::string &json_string) override;
    const t_global_config &getGlobalConfig() const;
    t_global_config fetchGlobalConfig();

    static t_global_config parseConfigFromFile(const std::string &conf_file_path);
};