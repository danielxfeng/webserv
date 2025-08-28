#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
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
    std::vector<std::string> createENVP(t_server_config server, std::unordered_map<std::string, std::string> headers);
public:
    CGIHandler();
    CGIHandler(const CGIHandler &copy);
    ~CGIHandler();
    CGIHandler &operator=(const CGIHandler &copy);

    //Setters
    std::vector<std::string> createENVP(t_server_config server, std::unordered_map<std::string, std::string> headers);


    //Getters
    std::string    getCGIOutput(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> headers);
};
