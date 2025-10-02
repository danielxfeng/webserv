#pragma once

#include <string>
#include <unordered_map>
#include "HttpRequests.hpp"
#include "HttpResponse.hpp"

const int MAX_COOKIE_AGE = 3600; // 1 hour

class Cookie
{
private:
    std::unordered_map<std::string, time_t> cookies_;
    bool checkValidAndExtendCookie(const std::string &cookie);
    std::string setCookie(std::string &cookie, time_t ts);

public:
    Cookie();
    Cookie(const Cookie &other) = default;
    Cookie &operator=(const Cookie &other) = default;
    ~Cookie();
    std::string set(HttpRequests &request);
};