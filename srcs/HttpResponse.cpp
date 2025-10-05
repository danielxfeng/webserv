#include "HttpResponse.hpp"
#include "HttpRequests.hpp"

/*
 HTTP/1.1 200 OK
 Date: Wed, 21 Aug 2025 12:34:56 GMT
 Content-Type: text/html
 Content-Length: 72

 */

std::string HttpResponse::successResponse(t_conn *conn, Cookie &cookie)
{
    std::string cookieStr = cookie.set(*conn->request);

    std::cout << "RUN fom Response" << std::endl;
    std::string res_target = conn->request->getrequestLineMap()["Target"];
    std::string content_type;
    if (!conn->res.isDynamic && res_target.find(".") != std::string::npos)
    {
        if (res_target.find(".png") != std::string::npos)
            content_type = "image/png";
        else if (res_target.find(".jpeg") != std::string::npos)
            content_type = "image/jpeg";
        else if (res_target.find(".jpg") != std::string::npos)
            content_type = "image/jpg";
        else if (res_target.find(".txt") != std::string::npos)
            content_type = "text/plain";
        else if (res_target.find(".html") != std::string::npos)
            content_type = "text/html";
        else
            content_type = "application/octet-stream";
    }
    else
        content_type = "text/html";

    std::string result;
    auto request = conn->request;
    if (request->getHttpRequestMethod() == "GET")
    {
        result.append("HTTP/1.1").append(" 200 OK\r\n");
        result.append("Content-Type: ").append(content_type).append("\r\n");
        result.append(cookieStr);
        result.append("Content-Length: ").append(std::to_string(conn->res.fileSize)).append("\r\n\r\n");
        if (conn->res.isDynamic)
            result.append(conn->res.dynamicPage);
    }
    else if (request->getHttpRequestMethod() == "POST")
    {
        result.append("HTTP/1.1").append(" 201 Created\r\n");
        result.append("Content-Type: ").append(content_type).append("\r\n");
        result.append("Location: ").append(conn->res.postFilename).append("\r\n");
        result.append(cookieStr);
        result.append("Content-Length: 0").append("\r\n\r\n");
    }
    else if (request->getHttpRequestMethod() == "DELETE")
    {
        std::string deleteSuccess = "<!DOCTYPE html>"
                                    "<html>"
                                    "<head><title>204 OK</title></head>"
                                    "<body>"
                                    "<h1>DELETE Success</h1>"
                                    "<p>The requested resource has been deleted.</p>"
                                    "</body>"
                                    "</html>";
        result.append("HTTP/1.1").append(" 204 OK\r\n");
        result.append("Content-Type: text/html\r\n");
        result.append(cookieStr);
        result.append("Content-Length: ").append(std::to_string(deleteSuccess.size())).append("\r\n\r\n");
        result.append(deleteSuccess);
    }
    std::cout << result << std::endl;
    return (result);
}

std::string HttpResponse::failedResponse(t_conn *conn, t_status_error_codes error_code, const std::string &error_message, size_t errPageSize, Cookie &cookie)
{
    std::string result;
    std::string status;
    auto request = conn->request;
    std::string cookieStr = cookie.set(*conn->request);

    switch (error_code)
    {
    case ERR_301_REDIRECT:
        status = "301 Moved Permanently";
        break;
    case ERR_400_BAD_REQUEST:
        status = "400 Bad Request";
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
    case ERR_405_METHOD_NOT_ALLOWED:
        status = "405 METHOD NOT ALLOWED";
        break;
    case ERR_409_CONFLICT:
        status = "409 Conflict";
        break;
    default:
        status = "500 Internal Server Error";
    }

    if (errPageSize == 0)
    {
        if (error_code == ERR_301_REDIRECT)
        {
            std::string redirected = error_message;

            result.append("HTTP/1.1").append(" ").append(status).append("\r\n");
            result.append("Location: ").append(redirected.erase(0, 12)).append("\r\n");
            return (result);
        }

        std::string htmlPage;
        htmlPage.append("<!DOCTYPE html>"
                        "<html>"
                        "<head><title>");
        htmlPage.append(status).append("</title></head><body><h1>");
        htmlPage.append(status).append("</h1><p>").append(error_message);
        htmlPage.append("</p></body></html>");
        result.append("HTTP/1.1").append(" ").append(status).append("\r\n");
        result.append("Content-Type: text/html\r\n");
        result.append(cookieStr);
        result.append("Connection: close\r\n");
        result.append("Content-Length: ").append(std::to_string(htmlPage.size())).append("\r\n\r\n");
        result.append(htmlPage);
    }
    else
    {
        result.append("HTTP/1.1").append(" ").append(status).append("\r\n");
        result.append("Content-Type: text/html\r\n");
        result.append(cookieStr);
        result.append("Connection: close\r\n");
        result.append("Content-Length: ").append(std::to_string(errPageSize)).append("\r\n\r\n");
    }
    return (result);
}
