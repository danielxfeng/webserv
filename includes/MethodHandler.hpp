#pragma once

#include "LogSys.hpp"
#include "WebServErr.hpp"
#include "utils.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <cstdio>
#include <unistd.h>
#include "SharedEnums.hpp"



class MethodHandler
{
private:
	int		fd_;
	size_t	expectedLength_;

	int		callGetMethod(std::string &path);
	int		callPostMethod(std::string &path, std::unordered_map<std::string, std::string> headers);
	void	callDeleteMethod(std::string &path);
	int		callCGIMethod(std::string &path, std::unordered_map<std::string, std::string> headers);

	void	setContentLength(std::unordered_map<std::string, std::string> headers);
	void	checkContentLength(std::unordered_map<std::string, std::string> headers) const;
	void	checkContentType(std::unordered_map<std::string, std::string> headers) const;
public:
    MethodHandler();
    MethodHandler(const MethodHandler &copy);
    ~MethodHandler();
    MethodHandler &operator=(const MethodHandler &copy);

	int	handleMethod(t_method method, std::unordered_map<std::string, std::string> headers);
};
