#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
	: requested_({-1, 0, 0, false, nullptr})
{
	LOG_TRACE("Method Handler created", (void));
}

MethodHandler::MethodHandler(const MethodHandler &copy)
{
    *this = copy;
}

MethodHandler::~MethodHandler()
{
	LOG_TRACE("Method Handler deconstructed", (void));
}

MethodHandler &MethodHandler::operator=(const MethodHandler &copy)
{
	if (this != &copy)
	{
		requested_.fd = copy.requested_.fd;
		requested_.expectedSize = copy.requested_.expectedSize;
		requested_.fileSize = copy.requested_.fileSize;
		requested_.isDynamic = copy.requested_.isDynamic;
		requested_.dynamicPage = copy.requested_.dynamicPage;
	}
	LOG_TRACE("Method handler copied", copy);
    return (*this);
}

t_file	MethodHandler::handleRequest(t_server_config server, std::unordered_map<std::string, std::string> headers)
{
	std::filesystem::path realPath = createRealPath(server.locations.root, headers.find("Target")->second);
	std::string chosenMethod = headers["Method"];
	if (server.locations.methods.find(chosenMethod) != server.locations.method.end())
	{
		if (chosenMethod == "GET")
		{
			LOG_TRACE("Calling GET: ", realPath);
			return (callGetMethod(realPath, server));
		}
		if (chosenMethod == "POST")
		{
			LOG_TRACE("Calling POST: ", realPath);
			return (callPostMethod(realPath, server, headers));
		}
		if (chosenMethod == "DELETE")
		{
			LOG_TRACE("Calling DELETE: ", realPath);
			callDeleteMethod(realPath);
			return (requested_);
		}
		if (chosenMethod == "CGI")
		{
			LOG_TRACE("Calling CGI: ", realPath);
			return (callCGIMethod(realPath, headers));
		}
	}
	else
	{
		LOG_WARN("Requested Method is not allowed: ", chosenMethod);
		throw WebServErr::MethodException(ERR_500, "Method not allowed or is unknown");
	}
}

t_file	MethodHandler::callGetMethod(std::filesystem::path &path, t_server_config server)
{
	checkIfLocExists(path);
	if (std::filesystem::is_directory(path))
	{
		std::string index_path = server.locations[path].index;//key is the path
		requested_.fd = open("../index/index.html", O_RDONLY | O_NONBLOCK);
		if (requested_.fd == -1)
		{
			LOG_FATAL("Failed to open file with permission: ", path);
			throw WebServErr::SysCallErrException("Failed to open file with permission");
		}
		requested_.fileSize = static_cast<int>(std::filesystem::file_size(index_path));
		return (requested_);
	}
	checkIfRegFile(path);
	checkIfSymlink(path);
	if (!access(path.c_str(), R_OK))
	{
		LOG_INFO("Permission Denied, cannot access: ", path);
		throw WebServErr::MethodException(ERR_404, "Permission denied to file");
	}
	requested_.fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
	if (requested_.fd == -1)
	{
		LOG_FATAL("Failed to open file with permission: ", path);
		throw WebServErr::SysCallErrException("Failed to open file with permission");
	}
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(path));
	return (requested_);
}

t_file	MethodHandler::callPostMethod(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> headers)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	//TODO Check if location has permission for POST
	auto typeCheck = headers.find("content-type");
	setContentLength(headers);
	if (typeCheck == headers.end())
	{
		LOG_ERROR("Bad Request, Type not found.", typeCheck);
		throw WebServErr::MethodException(ERR_400, "Bad Request, Type not found.");
	}
	if (typeCheck->second.find("multipart/form-data") == 0)
	{
		//TODO
		std::string type = headers.at("content-type");
		std::string start = "multipart/form-data; boundary=";
		if (type.find(start) == std::string::npos)
		{
			LOG_ERROR("Found no boundary data: ", type);
			WebServErr::MethodException(ERR_400, "Found no boundary data.");
		}
		std::string boundary = type.substr(start.size());
		//TODO Construct form data - check with Mohammad
	}
	else
	{

	}
	checkContentType(headers);
	//TODO get file name from headers
	std::string filename = createFileName(path.c_str());
	requested_.fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644);
	if (requested_.fd == -1)
	{
		LOG_FATAL("Failed to create: ", filename);
		throw WebServErr::SysCallErrException("Failed to create file");
	}
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(filename));
	return (requested_);
}

//Only throw if something is wrong, otherwise success is assumed
void	MethodHandler::callDeleteMethod(std::filesystem::path &path)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	checkIfRegFile(path);
	checkIfSymlink(path);
	if (!access(path.c_str(), X_OK))
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

t_file	MethodHandler::callCGIMethod(std::filesystem::path &path)
{
	//TODO
	return (requested_);
}

void	MethodHandler::setContentLength(std::unordered_map<std::string, std::string> headers)
{
	auto check = headers.find("content-length");
	if (check == headers.end())
	{
		LOG_ERROR("Content Length Required.", (void));
		throw WebServErr::MethodException(ERR_400, "Failed to get content length.");//TODO double check error code
	}
	std::stringstream length(check->second);
	length >> requested_.expectedSize;
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
	if (requested_.expectedSize > MAX_BODY_SIZE)
	{
		LOG_ERROR("Content length greater than max body size ", requested_.expectedSize);
		throw WebServErr::MethodException(/*TODO*/, "Body size too large.");
	}
}

void	MethodHandler::checkContentLength(std::unordered_map<std::string, std::string> headers) const
{
	auto check = headers.find("content-length");
	if (/*TODO length check here*/ != expectedLength_ || /*TODO*/ > MAX_BODY_SIZE)
	{
		LOG_ERROR("Bad Request, Body Length does not equal Expected Length ", check->second);
		throw WebServErr::MethodException(ERR_400, "Bad Request, Body Length does not equal Expected Length.");
	}
}

void	MethodHandler::checkContentType(std::unordered_map<std::string, std::string> headers) const
{
	auto typeCheck = headers.find("content-type");
	if (typeCheck->second.find("multipart/form-data"))
	{
		//TODO loop through parts and check MIMETYPE, if not throw bad request
	}
	else if (/*TODO chceck MIMEType of path against content type*/)
	{
		LOG_ERROR("Bad Request, Content Type does not match ", typeCheck->second);
		throw WebServErr::MethodException(ERR_400, "Bad Request, Content Type does not match.");
	}
}

void	MethodHandler::parseBoundaries(const std::string &boundary, std::vector<t_FormData>& sections)
{

}

void    MethodHandler::checkIfRegFile(const std::filesystem::path &path)
{
    if (!std::filesystem::is_regular_file(path))
	{
		LOG_ERROR("File is not a regular file: ", path);
		throw WebServErr::MethodException(ERR_500, "File is not a regular file");
	}
}

void	MethodHandler::checkIfSymlink(const std::filesystem::path &path)
{
	if (std::filesystem::is_symlink(path))
	{
		LOG_ERROR("File or Directory is a symlink: ", path);
		throw WebServErr::MethodException(ERR_500, "File or Directory is a symlink");
	}
}

void    MethodHandler::checkIfDirectory(const std::filesystem::path &path)
{
    if (std::filesystem::is_directory(path))
	{
		LOG_WARN("Target is a Directory: ", path);
		throw WebServErr::MethodException(ERR_403, "Target is a directory");
	}
}

void    MethodHandler::checkIfLocExists(const std::filesystem::path &path)
{
    if (!std::filesystem::exists(path))
	{
		LOG_ERROR("Permission Denied: Cannot Delete: ", path);
		throw WebServErr::MethodException(ERR_500, "Permission Denied, cannot delete selected file");
	}
}

std::filesystem::path	MethodHandler::createFileName(const std::string &path)
{
	//TODO trim .png
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	time_t tt = std::chrono::system_clock::to_time_t(now);
	tm	local_tm = *localtime(&tt);
	std::string year = std::to_string(local_tm.tm_year + 1900);
	std::string month = std::to_string(local_tm.tm_mon + 1);
	std::string day = std::to_string(local_tm.tm_mday);
	std::filesystem::path filename = /*TODO directory*/ path + '_' + year + '_' + month + '_' + day + '.png';
	checkIfLocExists(filename);
	return (filename);
}


std::filesystem::path	MethodHandler::createRealPath(const std::string &server, const std::string &target)
{
	std::filesystem::path root = std::filesystem::path(server);
	std::filesystem::path sought = std::filesystem::path(target).lexically_normal();
	if (sought.is_absolute())
		sought = sought.relative_path();
	if (!sought.empty() && root.filename() == (*sought.begin()))
		sought = sought.lexically_relative(*sought.begin());
	std::filesystem::path realPath = (root / sought).lexically_normal();

	std::filesystem::path canonicalRoot = std::filesystem::weakly_canonical(root);
	std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(realPath);
	auto mismatch = std::mismatch(canonicalRoot.begin(), canonicalRoot.end(), canonicalPath.begin());
	if (mismatch.first != canonicalRoot.end())
	{
		LOG_WARN("Security Warning: Path escapes server root ", realPath);
		throw WebServErr;MethodException(ERR_403, "Security Warning: Path escapes server root");
	}
	return (realPath);
}
