#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include "WebServErr.hpp"
#include "LogSys.hpp"
#include "SharedTypes.hpp"
#include "RaiiFd.hpp"

class CGIHandler
{
private:
    std::vector<std::string>	envp;
	t_file 		    	result;
	int		    	fds[2];


	//Setters
    void	setENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);

	//Processes
	void	handleWriteProcess(std::filesystem::path &script, std::filesystem::path &path);
	void	handleReadProcess(pid_t pid);
public:
    CGIHandler() = delete;
    CGIHandler(EpollHelper &epoll_helper);
    CGIHandler(const CGIHandler &copy) = delete;
    ~CGIHandler();
    CGIHandler &operator=(const CGIHandler &copy) = delete;

    //Getters
    t_file    getCGIOutput(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);
};
