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

#define READ 0
#define WRITE 1

class CGIHandler
{
private:
    std::vector<char*> envp;
    t_file result;

    // Setters
    void setENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);

    // Processes
    void handleCGIProcess(const std::filesystem::path &script, std::filesystem::path &path, int inPipe[2], int outPipe[2]);

public:
    CGIHandler() = delete;
    CGIHandler(EpollHelper &epoll_helper);
    CGIHandler(const CGIHandler &copy) = delete;
    ~CGIHandler();
    CGIHandler &operator=(const CGIHandler &copy) = delete;

    // Getters
    t_file getCGIOutput(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);
};
