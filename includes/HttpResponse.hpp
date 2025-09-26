#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <HttpRequests.hpp>
#include "Server.hpp"
#include "WebServErr.hpp"
#include "SharedTypes.hpp"

class HttpResponse
{

public:
    HttpResponse() = default;
    HttpResponse(const HttpResponse &obj) = default;
    HttpResponse &operator=(const HttpResponse &obj) = default;
    ~HttpResponse() = default;

    std::string successResponse(t_conn *conn);
    std::string failedResponse(t_conn *conn, t_status_error_codes error_code, const std::string &error_message);
};