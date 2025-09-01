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
	//TODO for everything in HEADER make uppercase and change - to _
	std::vector<std::string> data;
	data.emplace_back("SERVER_NAME=" + server.server_name);
	data.emplace_back("SERVER_PORT=" + std::to_string(server.port));
	data.emplace_back("REQUEST_METHOD=" + headers.find("Method")->second);
	data.emplace_back("PATH_INFO=" + headers.find("Target")->second);//TODO Do I need to send root & index?
	data.emplace_back("CONTENT_LENGTH=" + headers.find("content-length")->second);
	data.emplace_back("CONTENT_TYPE=" + headers.find("content-type")->second);//TODO Double check
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

t_file	CGIHandler::getCGIOutput(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> headers)
{
	std::vector<std::string> envp = createENVP(server, headers);
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
