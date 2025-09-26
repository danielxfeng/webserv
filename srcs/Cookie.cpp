# include "../includes/Cookie.hpp"

Cookie::Cookie() : cookies_() {}
Cookie::~Cookie() {}

bool Cookie::isValidCookie(const std::string &cookie)
{
    if (!cookies_.contains(cookie))
        return false;
    time_t current_time = time(nullptr);
    time_t cookie_time = cookies_[cookie];
    double seconds_diff = difftime(current_time, cookie_time);
    if (seconds_diff > MAX_COOKIE_AGE) // 1 hour expiration
    {
        cookies_.erase(cookie);
        return false;
    }
    return true;
}

std::string Cookie::setCookie(std::string &cookie, time_t ts)
{
    if (ts == 0) {
        ts = time(nullptr);
        cookies_.insert(std::make_pair(cookie, ts));
    }
    std::string cookie_str = "Set-Cookie: " + cookie + "; Max-Age=" + std::to_string(MAX_COOKIE_AGE) + "; HttpOnly\r\n";
    return (cookie_str);
}

std::string Cookie::set(HttpRequests &request)
{
    const auto headers = request.getrequestHeaderMap();
    const auto has_cookie = headers.contains("Cookie");
    const auto has_valid_cookie = has_cookie && isValidCookie(headers.at("Cookie"));
    if (has_valid_cookie)
    {
        auto cookie = headers.at("Cookie");
        return setCookie(cookie, cookies_[cookie]);
    }
    else
    {
        std::string new_cookie = "session_id=" + std::to_string(rand());
        return setCookie(new_cookie, 0);
    }
}