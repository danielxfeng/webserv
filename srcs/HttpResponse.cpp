#include "HttpResponse.hpp"
#include "HttpRequests.hpp"

   /*
    HTTP/1.1 200 OK
    Date: Wed, 21 Aug 2025 12:34:56 GMT
    Content-Type: text/html
    Content-Length: 72

    */



void HttpResponse::successResponse( t_conn conn, HttpRequests request,std::string &page_size)
{
    std::string result;
    if(request.getHttpRequestMethod() == "GET"){
        result.append(request.getHttpVersion()).append(" 202 OK\r\n");
        result.append("Content-Type: text/html\r\n");
        result.append("Content-Length: ").append(std::to_string(page_size.size())).append("\r\n\r\n");
    }
    else if(request.getHttpRequestMethod() == "POST"){
        result.append(request.getHttpVersion()).append(" 201 Created\r\n");
        result.append("Content-Type: text/html\r\n");
    }
    else if(request.getHttpRequestMethod() == "DELETE"){
        std::string result;
        std::string deleteSuccess ="<!DOCTYPE html>"
        "<html>"
        "<head><title>200 OK</title></head>"
        "<body>"
        "<h1>DELETE Success</h1>"
        "<p>The requested resource has been deleted.</p>"
        "</body>"
        "</html>";
    result.append(request.getHttpVersion()).append(" 200 OK\r\n");
    result.append("Content-Type: text/html\r\n");
    result.append("Content-Length: ").append(std::to_string(deleteSuccess.size())).append("\r\n\r\n");
    result.append(deleteSuccess);
    }
    write(conn.inner_fd_out, result.c_str(), result.size());
}

void HttpResponse::notFoundResponse( t_conn conn, HttpRequests request){
    std::string result;
    std::string notFound ="<!DOCTYPE html>"
        "<html>"
        "<head><title>404 Not Found</title></head>"
        "<body>"
        "<h1>404 Not Found</h1>"
        "<p>The requested resource was not found on this server.</p>"
        "</body>"
        "</html>";
    result.append(request.getHttpVersion()).append(" ").append("404 Not Found").append("\r\n");
    result.append("Content-Type: text/html\r\n");
    result.append("Content-Length: ").append(std::to_string(notFound.size())).append("\r\n\r\n");
    result.append(notFound);
     write(conn.inner_fd_out, result.c_str(), result.size());
}


void HttpResponse::badRequestResponse( t_conn conn, HttpRequests request){
    std::string result;
    std::string notFound ="<!DOCTYPE html>"
        "<html>"
        "<head><title>400 Bad Request</title></head>"
        "<body>"
        "<h1>400 Bad Request</h1>"
        "<p>The requested has problems.</p>"
        "</body>"
        "</html>";
    result.append(request.getHttpVersion()).append(" ").append("400 Bad Request").append("\r\n");
    result.append("Content-Type: text/html\r\n");
    result.append("Content-Length: ").append(std::to_string(notFound.size())).append("\r\n\r\n");
    result.append(notFound);
     write(conn.inner_fd_out, result.c_str(), result.size());
}