#include "../includes/ErrorResponse.hpp"

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

t_file  ErrorResponse::getErrorPage(std::unordered_map<t_status_error_codes, std::string> &errPages, t_status_error_codes &code)
{
    LOG_TRACE("Getting Error Response page for code: ", code);
    if (errPages.find(code) == errPages.end())
        throw WebServErr::ErrorResponseException("Error Page does not exist");
    std::filesystem::path errorFilename(errPages.find(code)->second);
    if (access(errorFilename.c_str(), R_OK) == -1)
        throw WebServErr::ErrorResponseException("Cannot access the error response page");
    response_.FD_handler_OUT->setFd(open(errorFilename.c_str(), O_RDONLY | O_NONBLOCK));
    if (response_.FD_handler_OUT.get()->get() == -1)
        throw WebServErr::ErrorResponseException("Failed to open the error response page");
    response_.fileSize = static_cast<int>(std::filesystem::file_size(errorFilename));
    LOG_TRACE("Returning Error Response via FD: ", response_.FD_handler_OUT);
    return (std::move(response_));
}
