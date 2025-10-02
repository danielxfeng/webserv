# include <algorithm>
# include "../includes/SharedTypes.hpp"

t_method convertMethod(const std::string &method_str)
{
    std::string upper_method = method_str;
    std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(), ::toupper);
    if (upper_method == "GET")
        return GET;
    else if (upper_method == "POST")
        return POST;
    else if (upper_method == "DELETE")
        return DELETE;
    else if (upper_method == "CGI")
        return CGI;
    else if (upper_method == "REDIRECT")
        return REDIRECT;
    else
        return UNKNOWN;
}