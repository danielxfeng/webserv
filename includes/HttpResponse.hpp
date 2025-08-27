#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <HttpRequests.hpp>
#include "Server.hpp"
#include "WebServErr.hpp"

class HttpResponse
{

    public:
        HttpResponse() = default;
        HttpResponse(const HttpResponse &obj)= default;
        HttpResponse& operator=(const HttpResponse &obj) = default;
        ~HttpResponse()= default;

        void successResponse(t_conn conn, HttpRequests request, std::string &statusCode);
        void badRequestResponse(t_conn conn, HttpRequests request);
        void notFoundResponse(t_conn conn, HttpRequests request);
        void failedResponse(HttpRequests request, t_conn *conn, t_status_error_codes error_code, const std::string &error_message);

        
};