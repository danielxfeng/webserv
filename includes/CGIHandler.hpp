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
#include <cctype>
#include <cstring>
#include <unistd.h>
#include "urlHelper.hpp"

#define READ 0
#define WRITE 1

class CGIHandler
{
private:
    std::vector<char*> envp;
    t_file result;

    // Setters
    void setENVP(const std::unordered_map<std::string, std::string> &requestLine, const std::unordered_map<std::string, std::string> &requestHeader);

    // Processes
    void handleCGIProcess(char **argv, std::filesystem::path &path, int inPipe[2], int outPipe[2]);
    std::filesystem::path getTargetCGI(const std::filesystem::path &path, t_server_config &server, bool *isPython);
    void checkRootValidity(const std::filesystem::path &root);

public:
    CGIHandler() = delete;
    CGIHandler(EpollHelper &epoll_helper);
    CGIHandler(const CGIHandler &copy) = delete;
    ~CGIHandler();
    CGIHandler &operator=(const CGIHandler &copy) = delete;

    // Getters
    t_file getCGIOutput(std::string &root, std::string &targetRef, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, t_server_config &server);
};
