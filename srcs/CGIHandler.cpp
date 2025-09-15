#include "../includes/CGIHandler.hpp"
#include <cctype>
#include <cstring>
#include <unistd.h>

CGIHandler::CGIHandler(EpollHelper &epoll_helper)
{
	result.FD_handler_IN = std::make_shared<RaiiFd>(epoll_helper);
	result.FD_handler_OUT = std::make_shared<RaiiFd>(epoll_helper);
	result.expectedSize = 0;
	result.fileSize = 0;
	result.isDynamic = false;
	fds[0] = -1;
	fds[1] = -1;
}

CGIHandler::~CGIHandler()
{
	for (char* ptr : envp)
	{
		delete [] ptr;
		ptr = nullptr;
	}
	envp.clear();
}

void CGIHandler::setENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	auto addToENVP = [this](const std::string &key, const std::string &value)
	{
		std::string temp;
		for (char c: key) 
		{
			if (c == '-')
				temp.push_back('_');
			else
				temp.push_back(std::toupper(static_cast<unsigned char>(c)));
		}
		temp.push_back('=');
		temp.append(value);
		char *cstr = new char[temp.size() + 1];
		std::strcpy(cstr, temp.c_str());
		envp.push_back(cstr);
	};
	// Sets requestLine into vector of strings
	for (const auto &key : requestLine)
	{
		addToENVP(key.first, key.second);
	}
	// Sets requestHeader into vector of strings
	for (const auto &key : requestHeader)
	{
		if (key.first == "boundary" && !key.second.empty())
		{
			std::string boundary = "BOUNDARY=" + key.second;
			char *cstr = new char[boundary.size() + 1];
			std::strcpy(cstr, boundary.c_str());
			envp.push_back(cstr);
		}
		else
		{
			addToENVP(key.first, key.second);
		}
	}
	// Sets requestBody into vector of strings
	for (const auto &key : requestBody)
	{
		addToENVP(key.first, key.second);
	}
	envp.push_back(nullptr);
}

void CGIHandler::handleCGIProcess(const std::filesystem::path &script, std::filesystem::path &path)
{
	char **final_envp = envp.data();
	std::string scriptStr = script.string();
	std::vector<char*> argv;
	argv.push_back(const_cast<char*>(scriptStr.c_str()));
	argv.push_back(nullptr);
	if (dup2(fds[1], STDIN_FILENO) == -1)
		throw WebServErr::CGIException("Dup2 Failure");
	close(fds[0]);
	if (execve(path.c_str(), argv.data(), final_envp) == -1)
		throw WebServErr::CGIException("Failed to execute CGI");
}

t_file CGIHandler::getCGIOutput(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	setENVP(requestLine, requestHeader, requestBody);
	const std::filesystem::path script = "/cgi_bin/python/cgi.py";
	if (!std::filesystem::exists(script))
		throw WebServErr::CGIException("CGI script does not exist.");
	if (std::filesystem::is_directory(script))
		throw WebServErr::CGIException("CGI script is a directory");
	if (std::filesystem::is_symlink(script))
		throw WebServErr::CGIException("CGI script is a symlink");
	if (!std::filesystem::is_regular_file(script))
		throw WebServErr::CGIException("CGI script is not a regular file.");
	//TODO Pipes
	int fds[2];
	result.fileDescriptor->setFd(socketpair(AF_UNIX, SOCK_STREAM, 0, fds));//TODO change
	pid_t pid = fork();
	if (pid == -1)
		throw WebServErr::CGIException("Failed to fork");
	if (pid == 0)
		handleCGIProcess(script, path);
	close(fds[1]);
	return (result);
}
