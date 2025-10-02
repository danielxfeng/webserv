#include "../includes/CGIHandler.hpp"


CGIHandler::CGIHandler(EpollHelper &epoll_helper)
{
	result.FD_handler_IN = std::make_shared<RaiiFd>(epoll_helper);
	result.FD_handler_OUT = std::make_shared<RaiiFd>(epoll_helper);
	result.expectedSize = 0;
	result.fileSize = 0;
	result.isDynamic = false;
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

void CGIHandler::setENVP(std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader)
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
	envp.push_back(nullptr);
}

void CGIHandler::handleCGIProcess(const std::filesystem::path &script, std::filesystem::path &path, int inPipe[2], int outPipe[2])
{
	char **final_envp = envp.data();
	std::string scriptStr = script.string();
	std::vector<char*> argv;
	argv.push_back(const_cast<char*>(scriptStr.c_str()));
	argv.push_back(nullptr);
	if (dup2(inPipe[READ], STDIN_FILENO) == -1)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Dup2 inPipe Failure");
	close(inPipe[WRITE]);
	if (dup2(outPipe[WRITE], STDOUT_FILENO) == -1)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Dup2 outPipe Failure");
	close(outPipe[READ]);
	if (execve(path.c_str(), argv.data(), final_envp) == -1)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Failed to execute CGI");
}

t_file CGIHandler::getCGIOutput(std::string &root, std::string &targetRef, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, t_server_config &server)
{
	setENVP(requestLine, requestHeader);
	
	//TODO create Path
	std::string rootDestination = matchLocation(server.locations, targetRef); // Find best matching location
	std::string root = server.locations[rootDestination].root;
	LOG_DEBUG("rootDestination: ", rootDestination);
	LOG_DEBUG("Root: ", root);
	std::string path = stripLocation(rootDestination, targetRef);
	
	// Create realPath
	std::filesystem::path realPath(root + path);
	LOG_DEBUG("Real Path: ", realPath);
	
	// Get Script
	const std::filesystem::path script = getTargetCGI(realPath, server);
	if (!std::filesystem::exists(script))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "CGI script does not exist.");
	if (std::filesystem::is_directory(script))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "CGI script is a directory");
	if (std::filesystem::is_symlink(script))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "CGI script is a symlink");
	if (!std::filesystem::is_regular_file(script))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "CGI script is not a regular file.");
	if  (access(script.c_str(), X_OK) == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "CGI script is not executable");

	int inPipe[2] = {-1, -1};
	int outPipe[2] = {-1, -1};
	if (pipe(inPipe) == -1)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "inPipe failed to initialize");
	if (pipe(outPipe) == -1)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "outPipe failed to initialize");
	result.FD_handler_IN->setFd(inPipe[WRITE]);
	result.FD_handler_OUT->setFd(outPipe[READ]);
	pid_t pid = fork();
	if (pid == -1)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "CGI Failed to fork");
	if (pid == 0)
		handleCGIProcess(script, realPath, inPipe, outPipe);
	close(inPipe[READ]);
	close(outPipe[WRITE]);
	return (result);
}

std::filesystem::path CGIHandler::getTargetCGI(const std::filesystem::path &path, t_server_config &server)
{
	std::string targetCGI;
	if (path.string().find("/cgi/python"))
		targetCGI = "python";
	else if (path.string().find("/cgi/go"))
		targetCGI = "go";
	else
		throw WebServErr::MethodException(ERR_501_NOT_IMPLEMENTED, "CGI extension not supported");
	if (server.cgi_paths.find(targetCGI) == server.cgi_paths.end())	
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "CGI extension does not exist");
	std::filesystem::path script(server.cgi_paths.find(targetCGI)->second);
	return (script);
}
