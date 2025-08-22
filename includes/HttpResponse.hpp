#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <HttpRequests.hpp>
#include "Server.hpp"


class HttpResponse{
    private:
        std::string date;
        std::string contentLength;
        std::string httpVersion;

    public:
        void successResponse(t_conn conn, HttpRequests request, std::string &statusCode);
        void faildResponse(t_conn conn, HttpRequests request, std::string &statusCode);




};