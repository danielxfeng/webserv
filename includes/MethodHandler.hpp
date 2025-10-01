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
#include <fstream>
#include <unistd.h>
#include <iomanip>
#include <ctime>
#include <random>
#include <unistd.h>

#include "SharedTypes.hpp"
#include "LogSys.hpp"
#include "WebServErr.hpp"
#include "Config.hpp"
#include "CGIHandler.hpp"
#include "RaiiFd.hpp"

#define MAX_BODY_SIZE 1024

typedef enum e_access
{
	READ_ONLY,
	WRITE_N_READ,
	EXECUTE,
}	t_access;

class MethodHandler
{
private:
	t_file requested_;

	t_file callGetMethod(bool useAutoIndex, std::filesystem::path &path, std::string &targetRef);
	t_file callPostMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestHeader, std::string &targetRef, const std::string &root);
	void callDeleteMethod(std::filesystem::path &path);
	t_file callCGIMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody, EpollHelper &epoll_helper);
	t_file callRedirectMethod(std::filesystem::path &path, t_server_config server, const std::string &root);

	void setContentLength(std::unordered_map<std::string, std::string> requestHeader);
	void checkContentType(std::unordered_map<std::string, std::string> requestBody) const;
	void checkIfRegFile(const std::filesystem::path &path);
	bool checkIfDirectory(std::unordered_map<std::string, t_location_config> &locations, std::filesystem::path &path, const std::string &rootDestination, const std::string &targetRef);
	void checkIfLocExists(const std::filesystem::path &path);
	bool checkIfSafe(const std::filesystem::path &root, const std::filesystem::path &path);
	size_t	checkFileCount(const std::string &root);

	std::string matchLocation(std::unordered_map<std::string, t_location_config> &locations, std::string &targetRef);
	std::string	stripLocation(const std::string &server, const std::string &target);
	std::filesystem::path createRealPath(const std::string &server, const std::string &target);
	std::filesystem::path createRandomFilename(std::filesystem::path &path, std::string &extension);
	std::string generateDynamicPage(std::filesystem::path &path, std::string &targetRef);
	bool	canAccess(std::filesystem::path &path, t_access access_type);

public:
	MethodHandler() = delete;
	MethodHandler(EpollHelper &epoll_helper);
	MethodHandler(const MethodHandler &copy) = delete;
	~MethodHandler();
	MethodHandler &operator=(const MethodHandler &copy) = delete;

	t_file handleRequest(t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody, EpollHelper &epoll_helper);
};
