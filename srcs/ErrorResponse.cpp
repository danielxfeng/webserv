#include "ErrorResponse.hpp"
#include <memory>

ErrorResponse::ErrorResponse(EpollHelper &epoll_helper)
{
    response_.FD_handler_IN = std::make_shared<RaiiFd>(epoll_helper);
    response_.FD_handler_OUT = std::make_shared<RaiiFd>(epoll_helper);
    response_.expectedSize = 0;
    response_.fileSize = 0;
    response_.isDynamic = false;
    LOG_TRACE("Error Reponse created ", "Go team go!");
}

ErrorResponse::~ErrorResponse()
{
    LOG_TRACE("Error Response ", "Deconstructed");
}

t_file  ErrorResponse::getErrorPage(t_server_config &server, t_status_error_codes &code)
{
    LOG_TRACE("Getting Error Reponse page for code: ", code);

}
