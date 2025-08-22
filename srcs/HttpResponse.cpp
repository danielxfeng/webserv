#include "HttpResponse.hpp"
#include "HttpRequests.hpp"



void HttpResponse::successResponse( t_conn conn, HttpRequests request,std::string &page_size)
{

    /*
    HTTP/1.1 200 OK
    Date: Wed, 21 Aug 2025 12:34:56 GMT
    Content-Type: text/html
    Content-Length: 72

    */
std::string result;
    if(request.getHttpRequestMethod() == "GET"){
        
        result.append(request.getHttpVersion()).append(" ").append("202 OK").append("\r\n");
        result.append("Content-Type: ").append("text/html").append("\r\n");
        result.append("Content-Length: ").append(std::to_string(page_size.size())).append("\r\n");
        std::cout<<"Debug:Result:\n"<<result;
    }
    else if(request.getHttpRequestMethod() == "POST" || request.getHttpRequestMethod() == "DELETE"){
        result.append(request.getHttpVersion()).append(" ").append("202 OK").append("\r\n");
        result.append("Content-Type: ").append("text/html").append("\r\n");
        result.append("Content-Length: ").append(std::to_string(page_size.size())).append("\r\n");
        std::cout<<"Debug:Result:\n"<<result;
       
    }
    write(conn.inner_fd_out, result.c_str(), result.size());
}