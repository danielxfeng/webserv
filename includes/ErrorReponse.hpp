#pragma once
#include "EpollHelper.hpp"
#include "SharedTypes.hpp"
#include "RaiiFd.hpp"
#include "WebServErr.hpp"
#include "LogSys.hpp"

class ErrorResponse
{
private: 
    t_file  response_;
public:
    ErrorResponse(EpollHelper &epoll_helper);
    ErrorResponse(const ErrorResponse &copy) = delete;
    ErrorResponse &operator= (const ErrorResponse &copy) = delete;
    ~ErrorResponse();

    t_file  getErrorPage(t_server_config &server, t_status_error_codes &code);
};
