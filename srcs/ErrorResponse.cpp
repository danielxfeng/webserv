#include "ErrorResponse.hpp"

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

t_file  ErrorResponse::getErrorPage(std::unordered_map<std::string, t_location_config> &locations, t_status_error_codes &code)
{
    LOG_TRACE("Getting Error Reponse page for code: ", code);
    std::string codeStr = errorCodeToString(code);
    std::string intenedRoot = "error/";
    //TODO get error root directory, if that fails get default error directory
    std::filesystem::path errorFilename = ;//TODO get file
    if (access(errorFilename.c_str(), R_OK) == -1)
        throw WebServErr::ErrorResponseException::ErrorResponseException("Cannot access the error response page");
    response_.FD_handler_OUT->setFd(open(errorFilename.c_str(), O_RDONLY | O_NONBLOCK));
    if (response_.FD_handler_OUT.get()->get() == -1)
        throw WebServErr::ErrorResponseException::ErrorResponseException("Failed to open the error reponse page");
    response_.fileSize = static_cast<int>(std::filesystem::file_size(errorFilename));
    LOG_TRACE("Returning Error Response via FD: ", response_.FD_handler_OUT);
    return (std::move(response_));
}

std::string ErrorResponse::errorCodeToString(t_status_error_codes &code){
    switch (code)
    {
        case (ERR_NO_ERROR)
            return ("");
        case (ERR_301_REDIRECT)
            return ("301");
        case (ERR_400_BAD_REQUEST)
            return ("400");
        case (ERR_401_UNAUTHORIZED)
            return ("401");
        case (ERR_403_FORBIDDEN)
            return ("403");
        case (ERR_404_NOT_FOUND)
            return ("404");
        case (ERR_405_METHOD_NOT_ALLOWED)
            return ("405");
        case (ERR_409_CONFLICT)
            return ("409");
        case (ERR_500_INTERNAL_SERVER_ERROR)
            return ("500");
        default:
            return ("");
    }
}
