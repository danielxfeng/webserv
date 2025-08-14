#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
	: fd_(-1);
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

//TODO Change to using filesystem
//TODO Throw rather than returning an error page
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
			throw WebServErr::MethodException("Unknown Method");
	}
}

int		MethodHandler::callGetMethod(std::string &path)
{
	std::string pathRef = //TODO May need to construct true path
	struct stat st;

	if (stat(pathRef.c_str(), &st) != 0)
	{
		LOG_WARN("File/Directory Not Found: ", pathRef);
		fd_ = open("../index/html/403.html", O_RDONLY);
		if (fd_ == -1)
		{
			LOG_ERR("Failed to get 403.html ", (void);
			throw WebServErr::MethodException(ERR_403, "Failed to get 403.html");
		}
		return (fd_);
	}
	if (S_ISDIR(st.st_mode))
	{
		LOG_WARN("Target is a Directory: ", pathRef);
		fd_ = open("../index/html/403.html", O_RDONLY);
		if (fd_ == -1)
		{
			LOG_ERR("Failed to get 403.html ", (void);
			throw WebServErr::MethodException(ERR_403, "Failed to get 403.html");
		}
		return (fd_);
	}
	if (!S_ISREG(st.st_mode))
	{
		LOG_WARN("File is not a regular file.", pathRef);
		fd_ = open("../index/html/403.html", O_RDONLY);
		if (fd_ == -1)
		{
			LOG_ERR("Failed to get 403.html ", (void);
			throw WebServErr::MethodException(ERR_403, "Failed to get 403.html");
		}
		return (fd_);
	}
	if (!access(pathRef, R_OK)
	{
		LOG_INFO("Permission Denied, cannot access: ", pathRef);
		fd_ = open("../index/html/404.html", O_RDONLY);
		if (fd_ == -1)
		{
			LOG_ERR("Failed to get 404.html ", (void);
			throw WebServErr::MethodException(ERR_404, "Failed to get 404.html");
		}
		return (fd_);
	}
	fd = open(pathRef.c_str(), O_RDONLY);
	if (fd_ == -1)
	{
		LOG_ERR("Failed to open file with permission: ", pathRef;
		throw WebServErr::MethodException(OTHER, "Failed to open file with permission");
	}
	return (fd_);
}

int		MethodHandler::callPostMethod(std::string &path)
{

	if (std::filesystem::is_directory())
	{
		LOG_WARN("Target is a Directory: ", pathRef);
		throw WebServErr::MethodException(ERR_403, "Target is a directory");
	}
	if (!std::filesystem::is_empty())
	{
		LOG_ERR("File already exists: " path);
		throw WebServErr::MethodException(/*TODO*/, "File already exists");
	}
	//TODO eed to check for multipart/form-data
	fd_ = open(pathRef.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);//TODO need to check if there is space for file
	if (fd_ == -1)
	{
		LOG_WARN("Failed to create: ", path);
		throw WebServErr::MethodException(ERR_403, "Failed to create file");
	}
	return (fd_);
}

//Only throw if something is wrong, otherwise success is assumed
void	MethodHandler::callDeleteMethod(std::string &path, std::unordered_map<std::string, std::string> headers)
{

	if (!std::filesystem::exists(path))
	{
		LOG_ERR("Permission Denied: Cannot Delete: ", path;
		throw WebServErr::MethodException(OTHER, "Permission Denied, cannot delete selected file");
	}
	if (!std::filesystem::is_regular_file(path))
	{
		LOG_ERR("File is not a regular file: ", path);
		throw WebServErr::MethodException(OTHER, "File is not a regular file");
	}
	if (!remove(path))
	{
		LOG_ERR("Failed to delete file: ", path);
		throw WebServErr::MethodException(OTHER, "Failed to delete selected file");
	}
}

int		MethodHandler::callCGIMethod(std::string &path)
{

	return (fd_);
}
