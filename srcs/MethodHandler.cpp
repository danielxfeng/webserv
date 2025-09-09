#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler(EpollHelper &epoll_helper)
{
	requested_.fileDescriptor = new RaiiFd(epoll_helper);
	requested_.expectedSize = 0;
	requested_.fileSize = 0;
	requested_.isDynamic = false;
	requested_.dynamicPage = nullptr;
	LOG_TRACE("Method Handler created", " Yay!");
}

MethodHandler::MethodHandler(const MethodHandler &copy)
{
	*this = copy;
}

MethodHandler::~MethodHandler()
{
	delete requested_.fileDescriptor;
	LOG_TRACE("Method Handler deconstructed", " Yay!");
}

MethodHandler &MethodHandler::operator=(const MethodHandler &copy)
{
	if (this != &copy)
	{
		requested_.fileDescriptor = copy.requested_.fileDescriptor;
		requested_.expectedSize = copy.requested_.expectedSize;
		requested_.fileSize = copy.requested_.fileSize;
		requested_.isDynamic = copy.requested_.isDynamic;
		requested_.dynamicPage = copy.requested_.dynamicPage;
	}
	LOG_TRACE("Method handler copied", " Yay!");
	return (*this);
}

t_file MethodHandler::handleRequest(t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
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
				return (callCGIMethod(realPath, requestLine, requestHeader, requestBody));
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
		requested_.fileDescriptor->setFd(open("../index/index.html", O_RDONLY | O_NONBLOCK));
		requested_.fileSize = static_cast<int>(std::filesystem::file_size(index_path));
		return (requested_);
	}
	checkIfRegFile(path);
	checkIfSymlink(path);
	if (!access(path.c_str(), R_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied, cannout GET file");
	requested_.fileDescriptor->setFd(open(path.c_str(), O_RDONLY | O_NONBLOCK));
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(path));
	return (std::move(requested_));
}

t_file MethodHandler::callPostMethod(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestBody)
{
	(void)server;
	(void)requestLine;
	checkIfLocExists(path);
	checkIfDirectory(path);
	if (!access(path.c_str(), W_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied, cannot POST file");
	auto typeCheck = requestBody.find("content-type");
	setContentLength(requestBody);
	if (typeCheck == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Type not found.");
	checkContentType(requestBody);
	std::filesystem::path filename = createPostFilename(path, requestBody);
	requested_.fileDescriptor->setFd(open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644));
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(filename));
	return (requested_);
}

std::filesystem::path MethodHandler::createPostFilename(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestBody)
{
	std::filesystem::path result = path;
	if (requestBody.find("filename") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Filename not found");
	result.append(createFileName(requestBody["filename"]->second));
	return (result);
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

t_file MethodHandler::callCGIMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	CGIHandler cgi;
	requested_ = cgi.getCGIOutput(path, requestLine, requestHeader, requestBody);
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
	if (requestBody.find("disposition-type") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, No Disposition Type");
	if (requestBody["disposition-type"]->second != "form-data")
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Wrong Disposition Type");
	if (requestBody.find("name") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, No Name Key");
	if (requestBody["name"]->second == "" || requestBody["name"]->second == nullptr)
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Name Is Empty String");
	if (requestBody.find("Content-Type") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, No Content Type");
	if (requestBody["Content-Type"]->second != "image/png" || requestBody["Content-Type"]->second == nullptr)
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Wrong Content Type");
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

std::string	MethodHandler::trimPath(const std::string &path)
{
	std::string result = path;
	const std::string extension = ".png";
	if (result.size() >= extension.size() && result.compare(path.size() - extension.size(), extension.size(), extension) == 0)
	{
		result.erase(result.size() - extension.size());
		LOG_TRACE("Trimmed filename: ", result);
	}
	return (result);
}

std::filesystem::path MethodHandler::createFileName(const std::string &path)
{
	std::string trimmedPath = trimPath(path);
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	time_t tt = std::chrono::system_clock::to_time_t(now);
	tm local_tm = *localtime(&tt);
	std::string year = std::to_string(local_tm.tm_year + 1900);
	std::string month = std::to_string(local_tm.tm_mon + 1);
	std::string day = std::to_string(local_tm.tm_mday);
	std::filesystem::path filename = "/index/user_content/" + trimmedPath + '_' + year + '_' + month + '_' + day + ".png";
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
