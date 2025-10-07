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
		if (ptr) {
            delete [] ptr;
			ptr = nullptr;
        }
	}
	envp.clear();
}

void CGIHandler::setENVP(
    const std::unordered_map<std::string, std::string> &requestLine,
    const std::unordered_map<std::string, std::string> &requestHeader
) {

	// Clear previous envp if any
	for (char* p : envp) if (p) delete [] p;
    envp.clear();

    auto addToENVP = [this](const std::string &key, const std::string &value) {
        std::string temp = key + "=" + value;
        char *cstr = new char[temp.size() + 1];
        std::strcpy(cstr, temp.c_str());
        envp.push_back(cstr);
    };

    for (const auto &kv : requestLine) {
        if (kv.first == "Method")
            addToENVP("REQUEST_METHOD", kv.second);
        else {
            std::string upper;
            for (char c : kv.first)
                upper.push_back(std::toupper((unsigned char)c));
            addToENVP(upper, kv.second);
        }
    }

    for (const auto &kv : requestHeader) {
        std::string keyUpper;
        keyUpper.reserve(kv.first.size());
        for (char c : kv.first) {
            if (c == '-') keyUpper.push_back('_');
            else keyUpper.push_back(std::toupper((unsigned char)c));
        }

        if (keyUpper == "CONTENT_TYPE")
            addToENVP("CONTENT_TYPE", kv.second);
        else if (keyUpper == "CONTENT_LENGTH")
            addToENVP("CONTENT_LENGTH", kv.second);
        else if (keyUpper == "HOST")
            continue;
        else if (keyUpper == "SERVERNAME" || keyUpper == "REQUESTPORT")
            continue;
        else
            addToENVP("HTTP_" + keyUpper, kv.second);
    }

    if (requestHeader.count("servername"))
        addToENVP("SERVER_NAME", requestHeader.at("servername"));
    if (requestHeader.count("requestport"))
        addToENVP("SERVER_PORT", requestHeader.at("requestport"));

    envp.push_back(nullptr);
}

void CGIHandler::handleCGIProcess(char **argv, std::filesystem::path &path, std::string &cmd, int inPipe[2], int outPipe[2])
{
	char **final_envp = envp.data();

	if (chdir(path.c_str()) == -1)
        return; // We cannot exit bc of the assignment
	close(inPipe[WRITE]);
    close(outPipe[READ]);
	if (dup2(inPipe[READ], STDIN_FILENO) == -1)
		return;
	if (dup2(outPipe[WRITE], STDOUT_FILENO) == -1)
		return;

	close(inPipe[READ]);
    close(outPipe[WRITE]);
	if (execve(cmd.c_str(), argv, final_envp) == -1)
		return;
}

std::string getProgName(std::string &targetRef)
{
	auto pos = targetRef.find("/cgi-bin/");
	if (pos == std::string::npos)
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, no cgi-bin found");
	return targetRef.substr(pos + 9);
}

std::string getExtName(std::string &prog_name)
{
	auto pos = prog_name.rfind('.');
	if (pos == std::string::npos || pos == prog_name.size() - 1)
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, no extension found");
	return prog_name.substr(pos);
}

t_file CGIHandler::getCGIOutput(std::string &targetRef, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, t_server_config &server)
{
	auto prog_name = getProgName(targetRef);
	auto ext_name = getExtName(prog_name);
	LOG_INFO("Program Name: ", prog_name, " Extension Name: ", ext_name);
	if (!server.cgi_paths.contains(ext_name))
		throw WebServErr::MethodException(ERR_501_NOT_IMPLEMENTED, "CGI extension not supported");

	const std::string interpreter = server.cgi_paths.at(ext_name).interpreter;
	const std::string root = server.cgi_paths.at(ext_name).root;

	const bool isInterpreter = !interpreter.empty();
	
	//Check Root Validity
	std::filesystem::path rootPath(root);
	checkRootValidity(rootPath);
	
	//Set up ENVP and ARGV
	setENVP(requestLine, requestHeader);
	// Set up ARGV
	std::vector<char*> argv;
	if (isInterpreter)
		argv.push_back(const_cast<char*>(interpreter.c_str()));
	argv.push_back(const_cast<char*>(prog_name.c_str()));
	argv.push_back(nullptr);
	
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

	std::string cmd = isInterpreter ? interpreter : prog_name;
	if (pid == 0)
	{
		handleCGIProcess(argv.data(), rootPath, cmd, inPipe, outPipe);
		return {}; // We cannot throw or exit in child process
	}
		
	int status;
	waitpid(pid, &status, WNOHANG);
	close(inPipe[READ]);
	close(outPipe[WRITE]);
	return (result);
}

// std::filesystem::path CGIHandler::getTargetCGI(const std::filesystem::path &path, t_server_config &server, bool *isPython)//TODO Make more robust
// {
// 	std::string targetCGI;
// 	if (path.string().find("/cgi/python"))
// 	{
// 		targetCGI = "python";
// 		*isPython = true;
// 	}
// 	else if (path.string().find("/cgi/go"))
// 	{
// 		targetCGI = "go";
// 		*isPython = false;
// 	}
// 	else
// 		throw WebServErr::MethodException(ERR_501_NOT_IMPLEMENTED, "CGI extension not supported");
// 	if (server.cgi_paths.find(targetCGI) == server.cgi_paths.end())	
// 		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "CGI extension does not exist");
// 	std::filesystem::path script(server.cgi_paths.find(targetCGI)->second);
// 	return (script);
// }

void	CGIHandler::checkRootValidity(const std::filesystem::path &root)
{
	LOG_INFO("Checking root validity: ", root.string());
	if (!std::filesystem::exists(root))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "CGI script does not exist.");
	if (!std::filesystem::is_directory(root))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "CGI script is not a directory");
	if (access(root.c_str(), R_OK | X_OK) == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "CGI script is not executable");
}

// void CGIHandler::checkCGIprograms(const t_server_config &server, const bool isPython)//TODO Make more robust
// {
// 	if (isPython)
// 	{
// 		if (!std::filesystem::exists("/usr/bin/python3"))
// 			throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Python not found");
// 		if (access("/usr/bin/python3", X_OK) == -1)
// 			throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Python is not accessible");
// 	}
// 	else
// 	{
// 		if (!std::filesystem::exists("usr/bin/go"))
// 			throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Go not found");
// 		if (access("/usr/bin/go", X_OK) == -1)
// 			throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Go is not accessible");
// 	}
// }