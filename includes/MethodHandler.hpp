#pragma once

#include "LogSys.hpp"
#include "WebServErr.hpp"
#include "Config.hpp"

#include <cstddef>
#include <cstdint>
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

typedef struct	s_fileinfo
{
	int	fd;
	size_t	expectedSize;
	size_t	fileSize;
	bool	isDynamic;
	std::string	dynamicPage;
}	t_file;

class MethodHandler
{
private:
	t_file	requested_;

	t_file		callGetMethod(std::string &path, t_server_config server);
	t_file		callPostMethod(std::string &path, t_server_config server, std::unordered_map<std::string, std::string> headers);
	void	callDeleteMethod(std::string &path);
	t_file		callCGIMethod(std::string &path, std::unordered_map<std::string, std::string> headers);

	void	setContentLength(std::unordered_map<std::string, std::string> headers);
	void	checkContentLength(std::unordered_map<std::string, std::string> headers) const;
	void	checkContentType(std::unordered_map<std::string, std::string> headers) const;
	void	parseBoundaries(const std::string &boundary, std::vector<t_FormData>& sections);
	void	checkIfRegFile(const std::string &path);
	void	checkIfSymlink(const std::string &path);
	void	checkIfDirectory(const std::string &path);
	void	checkIfLocExists(const std::string &path);
	std::string	createFileName(const std::string &path);
	std::string	createRealPath(const std::string &root, const std::string &path);
public:
    MethodHandler();
    MethodHandler(const MethodHandler &copy);
    ~MethodHandler();
    MethodHandler &operator=(const MethodHandler &copy);

	t_file	handleMethod(t_server_config server, std::unordered_map<std::string, std::string> headers);
};
