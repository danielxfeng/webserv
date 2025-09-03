#include "../includes/CGIHandler.hpp"

CGIHandler::CGIHandler()
	: result({-1,0, 0, false})
{
	fds[0] = -1;
	fds[1] = -1;
}

CGIHandler::CGIHandler(const CGIHandler &copy)
{
    *this = copy;
}

CGIHandler::~CGIHandler()
{}

CGIHandler &CGIHandler::operator=(const CGIHandler &copy)
{
    if (this != &copy)
    {
		envp = copy.envp;
		result.fd = copy.result.fd;
		result.dynamicPage = copy.result.dynamicPage;
		result.fileSize = copy.result.fileSize;
		result.expectedSize = copy.result.expectedSize;
		result.isDynamic = copy.result.isDynamic;
		fds[0] = copy.fds[0];
		fds[1] = copy.fds[1];
    }
    return (*this);
}

void	CGIHandler::setENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	// Sets requestLine into vector of strings
	for (auto key: requestLine)
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
	//Sets requestHeader into vector of strings
	for (auto key: requestHeader)
	{
		std::string temp;
		for(size_t i = 0; i < key.first.size(); i++)
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
				for(size_t k = 0; k < key.first.size(); k++)
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
	//Sets requestBody into vector of strings
	for (auto key: requestBody)
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

void	CGIHandler::handleWriteProcess(std::string script, std::filesystem::path &path, std::vector<std::string> &envp)
{
	if (dup2(fds[1], STDIN_FILENO) == -1)
		throw WebServErr::CGIException("Dup2 Failure");
	close(fds[0]);
	if (execve(path.c_str(), script, envp) == -1)
		throw WebServErr::CGIException("Failed to execute CGI");
}

void	CGIHandler::handleReadProcess(pid_t pid)
{
	close(fds[1]);
	std::string temp;
	constexpr size_t CHUNK_SIZE = 4096;
	while (true)
	{
		size_t oldSize = temp.size();
		temp.resize(oldSize + CHUNK_SIZE);
		ssize_t bytesRead = read(fds[0], &temp[oldSize], CHUNK_SIZE);
		if (bytesRead > 0)
			temp.resize(oldSize + bytesRead);
		else if (bytesRead == 0)
		{
			temp.resize(oldSize);
			break;
		}
		else
		{
			temp.resize(oldSize);
			if (errno == EINTR)
				continue ;
			throw WebServErr::CGIException("Reading from CGI failed.");
		}
	}
	result.fd = fds[0];
	result.dynamicPage = temp;
	result.fileSize = temp.size();
	result.expectedSize = temp.size();
	result.isDynamic = true;
}

t_file	CGIHandler::getCGIOutput(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	setENVP(requestLine, requestHeader, requestBody);
	std::string script = "../cgi_bin/python/cgi.py";
	if (!std::filesystem::exists(script))
		throw WebServErr::CGIException("CGI script does not exist.");
	if (std::filesystem::is_directory(script))
		throw WebServErr::CGIException("CGI script is a directory");
	if (std::filesystem::is_symlink(script))
		throw WebServErr::CGIException("CGI script is a symlink");
	if (!std::filesystem::is_regular_fille(script))
		throw WebServErr::CGIException("CGI script is not a regular file.");
	int		exit_code = 0;
	int fds[2];
	socketpair(AF_UNIX, SOCK_STREAM, fds);
	pid_t	pid = fork();
	if (pid == -1)
		throw WebServErr::CGIException("Failed to fork");
	if (pid == 0)
		handleWriteProcess(path, script, envp);
	else
		handleReadProcess(pid);
	close(fds[0]);
	waitpid(pid, &exit_code, 0);
	if (exit_code != 0)
		throw WebServErr::CGIException("Failed to complete external process");
	return (result);
}

