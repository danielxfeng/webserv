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
        std::vector<char> responseSerializer(HttpRequests request, std::string &page);
        std::string getTime();
};