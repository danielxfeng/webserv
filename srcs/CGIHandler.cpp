#include "../includes/CGIHandler.hpp"
#include <unordered_map>
#include <vector>

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

std::vector<std::string>    CGIHandler::createENVP(t_server_config server, std::unordered_map<std::string, std::string> headers)
{
	(void) server;
	(void) headers;
	std::vector<std::string> data;
	//TODO
	return (data);
}

std::string	CGIHandler::getCGIOutput(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> headers)
{
	std::vector<std::string> envp = createENVP(server, headers);
	//TODO Find CGI
	int		exit_code = 0;
	pid_t	pid = fork();
	std::string	result;
	if (pid == -1)
		throw WebServErr::CGIException("Failed to fork");
	if (pid == 0)
	{
		//TODO need args
		if (execve(path.c_str(), NULL, envp) == -1)
			throw WebServErr::CGIException("Failed to execute CGI");
		//TODO save CGI output to string
	}
	waitpid(pid, &exit_code, 0);
	//TODO check exit_code if (exit_code == )
		throw WebServErr::CGIException("Failed to complete external process");
	return (result);
}
