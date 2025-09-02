#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "WebServErr.hpp"
#include "LogSys.hpp"
#include "SharedTypes.hpp"

class CGIHandler
{
private:
    std::vector<std::string> envp;

	//Processes
	void	handleWriteProcess(std::filesystem::path &path, std::vector<std::string> &envp);
	void	handleReadProcess(pid_t pid);

	//Setters
    std::vector<std::string> createENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);
public:
    CGIHandler();
    CGIHandler(const CGIHandler &copy);
    ~CGIHandler();
    CGIHandler &operator=(const CGIHandler &copy);




    //Getters
    t_file    getCGIOutput(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);
};
