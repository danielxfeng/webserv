#pragma once

#include "LogSys.hpp"
#include "WebServErr.hpp"
#include <filesystem>
#include <string>

void	checkIfRegFile(const std::string &path);
void	checkIfDirectory(const std::string &path);
void	checkIfLocExists(const std::string &path);
