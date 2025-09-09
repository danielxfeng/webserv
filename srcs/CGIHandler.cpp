#include "../includes/CGIHandler.hpp"

CGIHandler::CGIHandler(EpollHelper &epoll_helper)
{
	result.fileDescriptor = std::make_shared<RaiiFd>(epoll_helper);
	result.expectedSize = 0;
	result.fileSize = 0;
	result.isDynamic = false;
	result.dynamicPage = nullptr;
	fds[0] = -1;
	fds[1] = -1;
}

CGIHandler::~CGIHandler()
{}

void CGIHandler::setENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	// Sets requestLine into vector of strings
	for (auto key : requestLine)
	{
		std::string temp;
		for (size_t h = 0; h < key.first.size(); h++)
		{
			if (key.first[h] == '-')
				temp.push_back('_');
			else
				temp.push_back(std::toupper(key.first.c_str()[h]));
		}
		temp.push_back('=');
		temp.append(key.second);
		envp.emplace_back(temp);
		temp.clear();
	}
	// Sets requestHeader into vector of strings
	for (auto key : requestHeader)
	{
		std::string temp;
		for (size_t i = 0; i < key.first.size(); i++)
		{
			if (key.first == "boundary" && !key.second.empty())
			{
				temp.append("BOUNDARY=");
				for (size_t j = 0; j < requestHeader["boundary"].size(); j++)
				{
					temp += requestHeader["boundary"][j];
					if (j + 1 != requestHeader["boundary"].size())
						temp.push_back(';');
				}
			}
			else
			{
				for (size_t k = 0; k < key.first.size(); k++)
				{
					if (key.first[k] == '-')
						temp.push_back('_');
					else
						temp.push_back(std::toupper(key.first.c_str()[k]));
				}
				temp.push_back('=');
				temp.append(key.second);
			}
		}
		envp.emplace_back(temp);
		temp.clear();
	}
	// Sets requestBody into vector of strings
	for (auto key : requestBody)
	{
		std::string temp;
		for (size_t l = 0; l < key.first.size(); l++)
		{
			if (key.first[l] == '-')
				temp.push_back('_');
			else
				temp.push_back(std::toupper(key.first.c_str()[l]));
		}
		temp.push_back('=');
		temp.append(key.second);
		envp.emplace_back(temp);
		temp.clear();
	}
}

void CGIHandler::handleCGIProcess(std::filesystem::path &script, std::filesystem::path &path)
{
	if (dup2(fds[1], STDIN_FILENO) == -1)
		throw WebServErr::CGIException("Dup2 Failure");
	close(fds[0]);
	if (execve(path.c_str(), script.c_str(), envp) == -1)
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
	int exit_code = 0;
	int fds[2];
	result.fileDescriptor->setFd(socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
	pid_t pid = fork();
	if (pid == -1)
		throw WebServErr::CGIException("Failed to fork");
	if (pid == 0)
		handleCGIProcess(script, path);
	else
	{
		close(fds[1]);
		return (std::move(result));
	}
}
