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
#include <unistd.h>

class ErrorResponse
{
private: 
    t_file  response_;

public:
    ErrorResponse() = delete;
    ErrorResponse(EpollHelper &epoll_helper);
    ErrorResponse(const ErrorResponse &copy) = delete;
    ErrorResponse &operator= (const ErrorResponse &copy) = delete;
    ~ErrorResponse();
    
    t_file  getErrorPage(std::unordered_map<t_status_error_codes, std::string> &errPages, t_status_error_codes &code);
};
