#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
	: fd_(-1), expectedLength_(0)
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
		expectedLength_ = copy.expectedLength_;
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
			return (callPostMethod(pathRef, headers));
		case DELETE:
			LOG_TRACE("Calling DELETE: ", pathRef);
			callDeleteMethod(pathRef);
			return (0);
		case CGI:
			LOG_TRACE("Calling CGI: ", pathRef);
			return (callCGIMethod(pathRef, headers));
		default:
			LOG_WARN("Method is unknown: ", headers);
			throw WebServErr::MethodException(ERR_500, "Unknown Method");
	}
}

int		MethodHandler::callGetMethod(std::string &path)
{
	checkIfLocExists(path);
	if (std::filesystem::is_directory(path))
	{
		fd_ = open("../index/index.html", O_RDONLY | O_NONBLOCK);
		if (fd_ == -1)
		{
			LOG_FATAL("Failed to open file with permission: ", path);
			throw WebServErr::SysCallErrException("Failed to open file with permission");
		}
		return (fd_);
	}
	checkIfRegFile(path);
	if (!access(path, R_OK))
	{
		LOG_INFO("Permission Denied, cannot access: ", path);
		throw WebServErr::MethodException(ERR_404, "Permission denied to file");
	}
	fd_ = open(path.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd_ == -1)
	{
		LOG_FATAL("Failed to open file with permission: ", path);
		throw WebServErr::SysCallErrException("Failed to open file with permission");
	}
	return (fd_);
}

int		MethodHandler::callPostMethod(std::string &path, std::unordered_map<std::string, std::string> headers)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	auto typeCheck = headers.find("Type");//TODO Check with Mohammad what this will be
	setContentLength(headers);
	if (typeCheck == headers.end())
	{
		LOG_ERROR("Bad Request, Type not found.", typeCheck);
		throw WebServErr::MethodException(ERR_400, "Bad Request, Type not found.");
	}
	if (typeCheck->second.find("multipart/form"))
	{
		//TODO
	}
	checkContentType(headers);
	fd_ = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644);
	if (fd_ == -1)
	{
		LOG_FATAL("Failed to create: ", path);
		throw WebServErr::SysCallErrException("Failed to create file");
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
		LOG_FATAL("Failed to delete file: ", path);
		throw WebServErr::SysCallErrException("Failed to delete selected file");
	}
}

int		MethodHandler::callCGIMethod(std::string &path)
{
	//TODO
	return (fd_);
}

void	MethodHandler::setContentLength(std::unordered_map<std::string, std::string> headers)
{
	auto check = headers.find("Length");
	if (check == headers.end())
	{
		LOG_ERROR("Content Length Required.", (void);
		throw WebServErr::MethodException(ERR_400, "Failed to get content length.");//TODO double check error code
	}
	std::stringstream length(check->second);
	length >> expectedLength_;
	if (length.fail())
	{
		LOG_ERROR("Content length not found", length);
		throw WebServErr::MethodException(ERR_400, "Bad Request, content length not found.");//TODO double check error code
	}
	if (!length.eof())
	{
		LOG_ERROR("Failed to get the end of file for content length ", length);
		throw WebServErr::MethodException(ERR_400, "Bad Request, failed to get end of file for content length.");//TODO double check error code
	}
	if (expectedLength_ > MAX_BODY_SIZE)//TODO set macro for max body size
	{
		LOG_ERROR("Content length greater than max body size ", expectedLength_);
		throw WebServErr::MethodException(/*TODO*/, "Body size too large.");
	}
}

void	MethodHandler::checkContentLength(std::unordered_map<std::string, std::string> headers) const
{
	auto check = headers.find("Length");
	if (/*TODO length check here*/)
	{
		LOG_ERROR("Bad Request, Body Length does not equal Expected Length ", check->second());
		throw WebServErr::MethodException(ERR_400, "Bad Request, Body Length does not equal Expected Length.");
	}
}

void	MethodHanlder::checkContentType(std::unordered_map<std::string, std::string> headers) const
{
	auto typeCheck = headers.find("Type");//TODO Check with Mohammad what this will be
	if (typeCheck->second.find("multipart/form"))
	{
		//TODO loop through parts and check MIMETYPE, if not throw bad request
	}
	else if (/*TODO chceck MIMEType of path against content type*/)
	{
		LOG_ERROR("Bad Request, Content Type does not match ", typecheck->second());
		throw WebServErr::MethodException(ERR_400, "Bad Request, Content Type does not match.");
	}
}
