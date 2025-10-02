#pragma once
#include <string>
#include <unordered_map>
#include "SharedTypes.hpp"
#include "LogSys.hpp"

std::string matchLocation(const std::unordered_map<std::string, t_location_config> &locations, const std::string &targetRef);
std::string	stripLocation(const std::string &server, const std::string &target);