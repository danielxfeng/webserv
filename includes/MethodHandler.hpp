#pragma once

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
#include <chrono>
#include <ctime>
#include <algorithm>

#include "SharedTypes.hpp"
#include "LogSys.hpp"
#include "WebServErr.hpp"
#include "Config.hpp"
#include "CGIHandler.hpp"
#include "RaiiFd.hpp"

#define MAX_BODY_SIZE 1024



class MethodHandler
{	
private:
	t_file requested_;

	t_file callGetMethod(bool useAutoIndex, std::filesystem::path &path);
	t_file callPostMethod(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody);
	void callDeleteMethod(std::filesystem::path &path);
	t_file callCGIMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody, EpollHelper &epoll_helper);

	void setContentLength(std::unordered_map<std::string, std::string> requestLine);
	void checkContentType(std::unordered_map<std::string, std::string> requestLine) const;
	void checkIfRegFile(const std::filesystem::path &path);
	void checkIfSymlink(const std::string &path);
	bool checkIfDirectory( std::unordered_map<std::string, t_location_config> &locations, const std::filesystem::path &path);
	void checkIfLocExists(const std::filesystem::path &path);

	std::string matchLocation(std::unordered_map<std::string, t_location_config> &locations, std::string &targetRef);
	std::string	trimPath(const std::string &path);
	std::filesystem::path createFileName(const std::string &path);
	std::filesystem::path createRealPath(const std::string &server, const std::string &target);
	std::filesystem::path createPostFilename(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestBody);
	std::string generateDynamicPage(std::filesystem::path &path);
public:
	MethodHandler() = delete;
	MethodHandler(EpollHelper &epoll_helper);
	MethodHandler(const MethodHandler &copy) = delete;
	~MethodHandler();
	MethodHandler &operator=(const MethodHandler &copy) = delete;

	t_file handleRequest(t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody, EpollHelper &epoll_helper);
};
