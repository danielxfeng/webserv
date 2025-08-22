#pragma once

#include "LogSys.hpp"
#include "WebServErr.hpp"
#include "Config.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <cstdio>
#include <unistd.h>
#include "SharedEnums.hpp"
#include <chrono>
#include <ctime>

#define MAX_BODY_SIZE	1024


typedef struct	s_FormData
{
	std::string	name_;
	std::string	type_;
	std::string	content_;
}	t_FormData;

typedef struct	s_FormData
{
	std::string	name_;
	std::string	type_;
	std::string	content_;
}	t_FormData;

class MethodHandler
{
private:
	int		file_details_[2];//[0] = file descriptor, [1] = file size
	size_t	expectedLength_;

	int*		callGetMethod(std::string &path, t_server_config server);
	int*		callPostMethod(std::string &path, t_server_config server, std::unordered_map<std::string, std::string> headers);
	void	callDeleteMethod(std::string &path);
	int*		callCGIMethod(std::string &path, std::unordered_map<std::string, std::string> headers);

	void	setContentLength(std::unordered_map<std::string, std::string> headers);
	void	checkContentLength(std::unordered_map<std::string, std::string> headers) const;
	void	checkContentType(std::unordered_map<std::string, std::string> headers) const;
	void	parseBoundaries(const std::string &boundary, std::vector<t_FormData>& sections);
	void	checkIfRegFile(const std::string &path);
	void	checkIfDirectory(const std::string &path);
	void	checkIfLocExists(const std::string &path);
	std::string	createFileName(const std::string &path);
public:
    MethodHandler();
    MethodHandler(const MethodHandler &copy);
    ~MethodHandler();
    MethodHandler &operator=(const MethodHandler &copy);

	int*	handleMethod(t_method method, t_server_config server, std::unordered_map<std::string, std::string> headers);
};
