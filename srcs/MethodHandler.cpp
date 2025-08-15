#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
	: fd_(-1)
{
	LOG_TRACE("Method Handler created", (void));
}

MethodHandler::MethodHandler(const MethodHandler &copy)
{
    *this = copy;
}

MethodHandler::~MethodHandler()
{}

MethodHandler &MethodHandler::operator=(const MethodHandler &copy)
{
    if (this != &copy)
    {
		fd_ = copy.fd_;
	}
	LOG_TRACE("Method handler copied", copy);
    return (*this);
}

/* Notes:
 * Headers includes 3 strings:
 *  Path
 *  Method
 *	Http Version
 *
 *	Request Header
 * 		Request length (how many
 *
 */

int		MethodHandler::handleMethod(t_method method, std::unordered_map<std::string, std::string> headers)
{
	std::string &pathRef = headers["Path"];
	switch (method)
	{
		case GET:
			LOG_TRACE("Calling GET: ", pathRef);
			return (callGetMethod(pathRef));
		case POST:
			LOG_TRACE("Calling POST: ", pathRef);
			return (callPostMethod(pathRef));
		case DELETE:
			LOG_TRACE("Calling DELETE: ", pathRef);
			callDeleteMethod(pathRef);
			return (0);
		case CGI:
			LOG_TRACE("Calling CGI: ", pathRef);
			return (callCGIMethod(pathRef));
		default:
			LOG_WARN("Method is unknown: ", headers);
			throw WebServErr::MethodException(OTHER, "Unknown Method");
	}
}

int		MethodHandler::callGetMethod(std::string &path)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	checkIfRegFile(path);
	if (!access(path, R_OK))
	{
		LOG_INFO("Permission Denied, cannot access: ", path);
		throw WebServErr::MethodException(ERR_404, "Permission denied to file");
	}
	fd_ = open(path.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd_ == -1)
	{
		LOG_ERR("Failed to open file with permission: ", path);
		throw WebServErr::MethodException(OTHER, "Failed to open file with permission");
	}
	return (fd_);
}

int		MethodHandler::callPostMethod(std::string &path)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	if (!std::filesystem::is_empty(path))
	{
		LOG_ERR("File already exists: ", path);
		throw WebServErr::MethodException(/*TODO*/, "File already exists");
	}
	//TODO eed to check for multipart/form-data
	fd_ = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644);//TODO need to check if there is space for file
	if (fd_ == -1)
	{
		LOG_WARN("Failed to create: ", path);
		throw WebServErr::MethodException(ERR_403, "Failed to create file");
	}
	return (fd_);
}

//Only throw if something is wrong, otherwise success is assumed
void	MethodHandler::callDeleteMethod(std::string &path)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	checkIfRegFile(path);
	if (!access(path, X_OK))
	{
		LOG_INFO("Permission Denied, cannot access: ", path);
		throw WebServErr::MethodException(ERR_404, "Permission denied to file");
	}
	if (!std::filesystem::remove(path))
	{
		LOG_ERR("Failed to delete file: ", path);
		throw WebServErr::MethodException(OTHER, "Failed to delete selected file");
	}
}

int		MethodHandler::callCGIMethod(std::string &path)
{
	//TODO
	return (fd_);
}
