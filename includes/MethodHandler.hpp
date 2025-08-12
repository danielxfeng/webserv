#pragma once

#include "LogSys.hpp"
#include "WebServErr.hpp"
#include <unordered_map>
#include <vector>
#include <string>

typedef enum e_method
{
    GET,
    POST,
    DELETE,
    CGI,
    UNKNOWN,
} t_method;

class MethodHandler
{
private:
	int		callGetMethod(std::unordered_map<std::string, std::string> headers);
	int		callPostMethod(std::unordered_map<std::string, std::string> headers);
	void	callDeleteMethod(std::unordered_map<std::string, std::string> headers);
	int		callCGIMethod(std::unordered_map<std::string, std::string> headers);
public:
    MethodHandler();
    MethodHandler(const MethodHandler &copy);
    ~MethodHandler();
    MethodHandler &operator=(const MethodHandler &copy);

	int		handleMethod(t_method method, std::unordered_map<std::string, std::string> headers);
};
