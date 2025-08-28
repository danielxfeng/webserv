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
#include "SharedTypes.hpp"
#include <chrono>
#include <ctime>
#include <algorithm>

#define MAX_BODY_SIZE 1024

typedef struct s_FormData
{
	std::string name_;
	std::string type_;
	std::string content_;
} t_FormData;

typedef struct s_fileinfo
{
	int fd;
	size_t expectedSize;
	size_t fileSize;
	bool isDynamic;
	std::string dynamicPage;
} t_file;

class MethodHandler
{
private:
	t_file requested_;

	t_file callGetMethod(std::filesystem::path &path, t_server_config server);
	t_file callPostMethod(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody);
	void callDeleteMethod(std::filesystem::path &path);
	t_file callCGIMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody);

	void setContentLength(std::unordered_map<std::string, std::string> requestLine);
	void checkContentLength(std::unordered_map<std::string, std::string> requestLine) const;
	void checkContentType(std::unordered_map<std::string, std::string> requestLine) const;
	void parseBoundaries(const std::string &boundary, std::vector<t_FormData> &sections);
	void checkIfRegFile(const std::filesystem::path &path);
	void checkIfSymlink(const std::filesystem::path &path);
	void checkIfDirectory(const std::filesystem::path &path);
	void checkIfLocExists(const std::filesystem::path &path);
	std::filesystem::path createFileName(const std::string &path);
	std::filesystem::path createRealPath(const std::string &server, const std::string &target);
	std::string generateDynamicPage(std::filesystem::path &path);

public:
	MethodHandler();
	MethodHandler(const MethodHandler &copy);
	~MethodHandler();
	MethodHandler &operator=(const MethodHandler &copy);

	t_file handleRequest(t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody);
};
