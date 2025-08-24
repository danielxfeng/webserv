#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <HttpRequests.hpp>
#include "Server.hpp"
#include "WebServErr.hpp"

class HttpResponse{

    public:
        void HttpResponse();
        void HttpResponse(const HttpResponse &obj);
        HttpResponse& operator=(const HttpResponse &obj);
        void ~HttpResponse();

        void successResponse(t_conn conn, HttpRequests request, std::string &statusCode);
        void badRequestResponse(t_conn conn, HttpRequests request);
        void notFoundResponse(t_conn conn, HttpRequests request);
        void failedResponse(HttpRequests request, t_conn *conn, t_status_error_codes error_code, const std::string &error_message);

};