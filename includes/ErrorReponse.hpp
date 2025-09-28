#pragma once
#include "EpollHelper.hpp"
#include "SharedTypes.hpp"
#include "RaiiFd.hpp"
#include "WebServErr.hpp"
#include "LogSys.hpp"
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <fcntl.h>

class ErrorResponse
{
private: 
    t_file  response_;

    std::string errorCodeToString(t_status_error_codes &code);
    std::filesystem::path getFilename(std::string &root, std::string &codeStr);
public:
    ErrorResponse() = delete;
    ErrorResponse(EpollHelper &epoll_helper);
    ErrorResponse(const ErrorResponse &copy) = delete;
    ErrorResponse &operator= (const ErrorResponse &copy) = delete;
    ~ErrorResponse();

    t_file  getErrorPage(std::unordered_map<std::string, t_location_config> &locations, t_status_error_codes &code);
};
