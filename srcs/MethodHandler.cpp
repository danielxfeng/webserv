#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
	: requested_({-1, 0, 0, false, nullptr})
{
	LOG_TRACE("Method Handler created", " Yay!");
}

MethodHandler::MethodHandler(const MethodHandler &copy)
{
	*this = copy;
}

MethodHandler::~MethodHandler()
{
	LOG_TRACE("Method Handler deconstructed", " Yay!");
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
	LOG_TRACE("Method handler copied", " Yay!");
	return (*this);
}

t_file MethodHandler::handleRequest(t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody)
{
	std::string targetRef = requestLine.find("Target") != requestLine.end() ? targetRef = requestLine.find("Target")->second : "";
	if (targetRef == "")
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Path does not exist");
	std::string chosenMethod = requestLine.find("Method") != requestLine.end() ? chosenMethod = requestLine.find("Method")->second : "";
	if (chosenMethod == "")
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Method does not exist");
	t_method realMethod = convertMethod(chosenMethod);
	std::string &rootRef = server.locations[targetRef].root;
	std::filesystem::path realPath = createRealPath(rootRef, targetRef);

	for (size_t i = 0; i < server.locations[targetRef].methods.size(); i++)
	{
		if (server.locations[targetRef].methods[i] == realMethod)
		{
			switch (realMethod)
			{
			case GET:
			{
				LOG_TRACE("Calling GET: ", realPath);
				return (callGetMethod(realPath, server));
			}
			case POST:
			{
				LOG_TRACE("Calling POST: ", realPath);
				return (callPostMethod(realPath, server, requestLine, requestBody));
			}
			case DELETE:
			{
				LOG_TRACE("Calling DELETE: ", realPath);
				callDeleteMethod(realPath);
				return (requested_);
			}
			case CGI:
			{
				LOG_TRACE("Calling CGI: ", realPath);
				return (callCGIMethod(realPath, requestLine, requestBody));
			}
			default:
				throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Method not allowed or is unknown");
			}
		}
	}
	throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Method not allowed or is unknown");
}

t_file MethodHandler::callGetMethod(std::filesystem::path &path, t_server_config server)
{
	checkIfLocExists(path);
	if (std::filesystem::is_directory(path))
	{
		std::filesystem::path index_path = std::filesystem::path(server.locations[path].index);
		if (index_path == "")
		{
			requested_.dynamicPage = generateDynamicPage(path);
			requested_.isDynamic = true;
			requested_.fileSize = requested_.dynamicPage.size();
			return (requested_);
		}
		requested_.fd = open("../index/index.html", O_RDONLY | O_NONBLOCK);
		if (requested_.fd == -1)
			throw WebServErr::SysCallErrException("Failed to open file with permission");
		requested_.fileSize = static_cast<int>(std::filesystem::file_size(index_path));
		return (requested_);
	}
	checkIfRegFile(path);
	checkIfSymlink(path);
	if (!access(path.c_str(), R_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied to file");
	requested_.fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
	if (requested_.fd == -1)
		throw WebServErr::SysCallErrException("Failed to open file with permission");
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(path));
	return (requested_);
}

t_file MethodHandler::callPostMethod(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody)
{
	(void)server;
	(void)requestLine;
	checkIfLocExists(path);
	checkIfDirectory(path);
	// TODO Check if location has permission for POST
	auto typeCheck = requestBody.find("content-type");
	setContentLength(requestBody);
	if (typeCheck == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Type not found.");
	checkContentType(requestBody);
	// TODO get file name from requestBody
	std::string filename = createFileName(path.c_str());
	requested_.fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644);
	if (requested_.fd == -1)
		throw WebServErr::SysCallErrException("Failed to create file");
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(filename));
	return (requested_);
}

// Only throw if something is wrong, otherwise success is assumed
void MethodHandler::callDeleteMethod(std::filesystem::path &path)
{
	checkIfLocExists(path);
	checkIfDirectory(path);
	checkIfRegFile(path);
	checkIfSymlink(path);
	if (!access(path.c_str(), X_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied to file");
	if (!std::filesystem::remove(path))
		throw WebServErr::SysCallErrException("Failed to delete selected file");
}

t_file MethodHandler::callCGIMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody)
{
	(void)path;
	(void)requestLine;
	(void)requestBody;
	// TODO
	return (requested_);
}

void MethodHandler::setContentLength(std::unordered_map<std::string, std::string> requestBody)
{
	auto check = requestBody.find("content-length");
	if (check == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Failed to get content length."); // TODO double check error code
	std::stringstream length(check->second);
	length >> requested_.expectedSize;
	if (length.fail())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, content length not found."); // TODO double check error code
	if (!length.eof())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, failed to get end of file for content length."); // TODO double check error code
	if (requested_.expectedSize > MAX_BODY_SIZE)
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Body size too large."); // TODO double check error code
}

void MethodHandler::checkContentType(std::unordered_map<std::string, std::string> requestBody) const
{
	auto typeCheck = requestBody.find("content-type");
	if (typeCheck->second.find("multipart/form-data"))
	{
		// TODO loop through parts and check MIMETYPE, if not throw bad request
	}
	//	else if (/*TODO chceck MIMEType of path against content type*/)
	//		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Content Type does not match.");
}

void MethodHandler::parseBoundaries(const std::string &boundary, std::vector<t_FormData> &sections)
{
	(void)boundary;
	(void)sections;
}

void MethodHandler::checkIfRegFile(const std::filesystem::path &path)
{
	if (!std::filesystem::is_regular_file(path))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File is not a regular file");
}

void MethodHandler::checkIfSymlink(const std::filesystem::path &path)
{
	if (std::filesystem::is_symlink(path))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File or Directory is a symlink");
}

void MethodHandler::checkIfDirectory(const std::filesystem::path &path)
{
	if (std::filesystem::is_directory(path))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Target is a directory");
}

void MethodHandler::checkIfLocExists(const std::filesystem::path &path)
{
	if (!std::filesystem::exists(path))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Permission Denied, cannot delete selected file");
}

std::filesystem::path MethodHandler::createFileName(const std::string &path)
{
	// TODO trim .png
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	time_t tt = std::chrono::system_clock::to_time_t(now);
	tm local_tm = *localtime(&tt);
	std::string year = std::to_string(local_tm.tm_year + 1900);
	std::string month = std::to_string(local_tm.tm_mon + 1);
	std::string day = std::to_string(local_tm.tm_mday);
	std::filesystem::path filename = /*TODO directory*/ path + '_' + year + '_' + month + '_' + day + ".png";
	checkIfLocExists(filename);
	return (filename);
}

std::filesystem::path MethodHandler::createRealPath(const std::string &server, const std::string &target)
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
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Security Warning: Path escapes server root");
	return (realPath);
}

std::string MethodHandler::generateDynamicPage(std::filesystem::path &path)
{
	std::string str;
	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		str += entry.path();
		str += ' ';
	}
	str += '\0';
	return (str);
}
