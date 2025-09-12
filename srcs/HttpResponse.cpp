#include "HttpResponse.hpp"
#include "HttpRequests.hpp"

/*
 HTTP/1.1 200 OK
 Date: Wed, 21 Aug 2025 12:34:56 GMT
 Content-Type: text/html
 Content-Length: 72

 */

std::string HttpResponse::successResponse(t_conn *conn, std::size_t &page_size)
{
    std::string result;
    auto request = conn->request;
    if (request->getHttpRequestMethod() == "GET")
    {
        result.append(request->getHttpVersion()).append(" 202 OK\r\n");
        result.append("Content-Type: text/html\r\n");
        result.append("Content-Length: ").append(std::to_string(page_size)).append("\r\n\r\n");
    }
    else if (request->getHttpRequestMethod() == "POST")
    {
        result.append(request->getHttpVersion()).append(" 201 Created\r\n");
        result.append("Content-Type: text/html\r\n");
    }
    else if (request->getHttpRequestMethod() == "DELETE")
    {
        std::string result;
        std::string deleteSuccess = "<!DOCTYPE html>"
                                    "<html>"
                                    "<head><title>200 OK</title></head>"
                                    "<body>"
                                    "<h1>DELETE Success</h1>"
                                    "<p>The requested resource has been deleted.</p>"
                                    "</body>"
                                    "</html>";
        result.append(request->getHttpVersion()).append(" 200 OK\r\n");
        result.append("Content-Type: text/html\r\n");
        result.append("Content-Length: ").append(std::to_string(deleteSuccess.size())).append("\r\n\r\n");
        result.append(deleteSuccess);
    }
    return (result);
}

std::string HttpResponse::notFoundResponse(t_conn *conn)
{
    std::string result;
    auto request = conn->request;
    std::string notFound = "<!DOCTYPE html>"
                           "<html>"
                           "<head><title>404 Not Found</title></head>"
                           "<body>"
                           "<h1>404 Not Found</h1>"
                           "<p>The requested resource was not found on this server.</p>"
                           "</body>"
                           "</html>";
    result.append(request->getHttpVersion()).append(" ").append("404 Not Found").append("\r\n");
    result.append("Content-Type: text/html\r\n");
    result.append("Content-Length: ").append(std::to_string(notFound.size())).append("\r\n\r\n");
    result.append(notFound);
    return (result);
}

std::string HttpResponse::badRequestResponse(t_conn *conn)
{
    std::string result;
    auto request = conn->request;
    std::string notFound = "<!DOCTYPE html>"
                           "<html>"
                           "<head><title>400 Bad Request</title></head>"
                           "<body>"
                           "<h1>400 Bad Request</h1>"
                           "<p>The requested has problems.</p>"
                           "</body>"
                           "</html>";
    result.append(request->getHttpVersion()).append(" ").append("400 Bad Request").append("\r\n");
    result.append("Content-Type: text/html\r\n");
    result.append("Content-Length: ").append(std::to_string(notFound.size())).append("\r\n\r\n");
    result.append(notFound);
    return (result);
}

/*
ERR_301,
    ERR_401,
    ERR_403,
    ERR_404,
    ERR_409,
    ERR_500
    */

std::string HttpResponse::failedResponse(t_conn *conn, t_status_error_codes error_code, const std::string &error_message)
{
    std::string result;
    std::string status;
    auto request = conn->request;

    switch (error_code)
    {
    case ERR_301_REDIRECT:
        status = "301 Moved Permanently";
        break;
    case ERR_401_UNAUTHORIZED:
        status = "401 Unauthorized";
        break;
    case ERR_403_FORBIDDEN:
        status = "403 Forbidden";
        break;
    case ERR_404_NOT_FOUND:
        status = "404 Not Found";
        break;
    case ERR_409_CONFLICT:
        status = "409 Conflict";
        break;
    case ERR_500_INTERNAL_SERVER_ERROR:
        status = "500 Internal Server Error";
        break;

    default:
        break;
    }
    std::string htmlPage;
    htmlPage.append("<!DOCTYPE html>"
                    "<html>"
                    "<head><title>");
    htmlPage.append(status).append("</title></head><body><h1>").append(status).append("</h1><p>").append(error_message).append("</p></body></html>");

    result.append(request->getHttpVersion()).append(" ").append(status).append("\r\n");
    result.append("Content-Type: text/html\r\n");
    result.append("Content-Length: ").append(std::to_string(htmlPage.size())).append("\r\n\r\n");
    result.append(htmlPage);

    return (result);
}
