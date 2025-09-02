#include "../includes/CGIHandler.hpp"

CGIHandler::CGIHandler()
{}

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

    }
    return (*this);
}

std::vector<std::string>    CGIHandler::createENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	std::vector<std::string> data;
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
		data.emplace_back(temp);
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
		data.emplace_back(temp);
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
		data.emplace_back(temp);
		temp.clear();
	}
	return (data);
}

void	CGIHandler::handleWriteProcess(std::filesystem::path &path, std::vector<std::string> &envp)
{
	//TODO need args
	if (execve(path.c_str(), NULL, envp) == -1)
		throw WebServErr::CGIException("Failed to execute CGI");
	//TODO save CGI output to string
}

void	CGIHandler::handleReadProcess(pid_t pid)
{

}

t_file	CGIHandler::getCGIOutput(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	std::vector<std::string> envp = createENVP(requestLine, requestHeader, requestBody);
	t_file result = {-1,0, 0, false};
	//TODO Find CGI
	int		exit_code = 0;
	//TODO Do we need a pipe?
	pid_t	pid = fork();
	if (pid == -1)
		throw WebServErr::CGIException("Failed to fork");
	if (pid == 0)
	{
		handleWriteProcess(path, envp);
	}
	else
	{
		handleReadProcess(pid);
	}
	waitpid(pid, &exit_code, 0);
	//TODO check exit_code if (exit_code == )
		throw WebServErr::CGIException("Failed to complete external process");
	return (result);
}

/*NOTES:
 * For POST CGI, need a pipe and pass the write FD to HTTP Response, while read FD goes to CGI file
 * For GET CGI, need a pipe and pass the write FD to CGI File, while read FD goes to Parent process
 * Use socketpair
 * /
