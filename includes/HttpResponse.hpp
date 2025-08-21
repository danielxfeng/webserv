#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iostream>
#include <HttpRequests.hpp>



class HttpResponse{
    private:
        std::string date;
        std::string contentLength;
        std::string httpVersion;

    public:
        std::string successResponse(HttpRequests request, std::string &statusCode);
        std::string successFailed(HttpRequests request, std::string &statusCode);
        std::string getTime();



};