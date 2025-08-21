#include "HttpResponse.hpp"
#include "HttpRequests.hpp"





std::string HttpResponse::getTime()
{
    std::ostringstream oss; // here we create ostringstream.
	std::time_t result = std::time(nullptr);
    oss << std::asctime(std::localtime(&result));
    return (oss.str()); // return oss.str().
}


std::string HttpResponse::successResponse(HttpRequests request,std::string &page_size)
{

    /*
    HTTP/1.1 200 OK
    Date: Wed, 21 Aug 2025 12:34:56 GMT
    Content-Type: text/html
    Content-Length: 72

    #define SUCCESS 202 OK;
    #define NOTFOUND 404 Not Found;
    #define BADREQUEST 400 Bad Request;
    */

    if(request.getHttpRequestMethod() == "GET"){
        std::string result;
        result.append(request.getHttpVersion()).append(" ").append("202 OK").append("\r\n");
        result.append("Date: ").append(getTime());
        result.append("Content-Type: ").append("text/html").append("\r\n");
        result.append("Content-Length: ").append(std::to_string(page_size.size())).append("\r\n");
        std::cout<<"Debug:Result:\n"<<result;
        return (result);
    }
    else if(request.getHttpRequestMethod() == "POST" || request.getHttpRequestMethod() == "DELETE"){
        std::string result;
        result.append(request.getHttpVersion()).append(" ").append("202 OK").append("\r\n");
        result.append("Date: ").append(getTime());
        result.append("Content-Type: ").append("text/html").append("\r\n");
        result.append("Content-Length: ").append(std::to_string(page_size.size())).append("\r\n");
        std::cout<<"Debug:Result:\n"<<result;
        return (result);
    }
    
}