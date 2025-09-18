#include "../includes/MethodHandler.hpp"
#include <filesystem>

MethodHandler::MethodHandler(EpollHelper &epoll_helper)
{
	requested_.fileDescriptor = std::make_shared<RaiiFd>(epoll_helper);
	requested_.expectedSize = 0;
	requested_.fileSize = 0;
	requested_.isDynamic = false;
	//requested_.dynamicPage = nullptr;
	LOG_TRACE("Method Handler created", " Yay!");
}

MethodHandler::~MethodHandler()
{
	LOG_TRACE("Method Handler deconstructed", " Yay!");
}

t_file MethodHandler::handleRequest(t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody, EpollHelper &epoll_helper)
{
	LOG_TRACE("Handle Request Started: ", "Let's see what happens...");
	std::string targetRef;

	if (requestLine.find("Target") != requestLine.end())
	{
		targetRef = requestLine["Target"];
		LOG_TRACE("Target found: ", targetRef);
	}
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Http Target does not exist");
	std::string chosenMethod;
	std::cout << requestLine["Method"] << std::endl;
	if (requestLine.find("Method") != requestLine.end())
	{
		chosenMethod = requestLine["Method"];
		LOG_TRACE("Method found: ", chosenMethod);
	}
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Http Method does not exist");
	std::string rootDestination = matchLocation(server.locations, targetRef);// Find best matching location
	std::string targetPath = rootDestination + targetRef;
	LOG_TRACE("Target Path: ", targetPath);
	checkIfSymlink(targetPath);
	std::filesystem::path realPath = createRealPath(rootDestination, targetRef);
	checkIfLocExists(realPath);
	bool useAutoIndex = checkIfDirectory(server.locations, realPath);//TODO do we need a check for auto-index or can we just do it automatically?
	if (!useAutoIndex)
		checkIfRegFile(realPath);
	t_method realMethod = convertMethod(chosenMethod);
	for (size_t i = 0; i < server.locations[rootDestination].methods.size(); i++)
	{
		std::cout << "Server methods: " << server.locations[rootDestination].methods[i] << std::endl;
		if (server.locations[rootDestination].methods[i] == realMethod)
		{
			switch (realMethod)
			{
				case GET:
					return (callGetMethod(useAutoIndex, realPath));
				case POST:
					return (callPostMethod(realPath, server, requestLine, requestHeader, requestBody));
				case DELETE:
				{
					callDeleteMethod(realPath);
					return (requested_);
				}
				case CGI:
					return (callCGIMethod(realPath, requestLine, requestHeader, requestBody, epoll_helper));
				default:
					throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Method not allowed or is unknown");
			}
		}
	}
	throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Handle Request failing everything!");
}

std::string	MethodHandler::matchLocation(std::unordered_map<std::string, t_location_config> &locations, std::string &targetRef)
{
	LOG_TRACE("Matching Location for: ", targetRef);
	std::string bestMatch = "";
	size_t	longestMatchLength = 0;
	std::string normalizedTargetPath = targetRef;
	if (normalizedTargetPath.back() != '/')
			normalizedTargetPath += '/';
	for (auto it = locations.begin();  it != locations.end(); it++)
	{
		const std::string &locPath = it->second.root;
		std::string normalizedLocPath = locPath;
		if (normalizedLocPath.back() != '/')
			normalizedLocPath += '/';
		if (normalizedTargetPath.find(normalizedLocPath) == 0)
		{
			if (normalizedLocPath.length() > longestMatchLength)
			{
				bestMatch = it->second.root;
				longestMatchLength = normalizedLocPath.length();
			}
		}
	}
	if (bestMatch == "")
		bestMatch = "/";
	LOG_TRACE("Best Location Match: ", bestMatch);
	return (bestMatch);
}

t_file MethodHandler::callGetMethod(bool useAutoIndex, std::filesystem::path &path)
{
	LOG_TRACE("Calling GET: ", path);
	if (useAutoIndex)
	{
		LOG_TRACE("Directory is: ", "auto-index");
		//TODO Deal with FD, probably need a pipe
		requested_.dynamicPage = generateDynamicPage(path);
		requested_.isDynamic = true;
		requested_.fileSize = requested_.dynamicPage.size();
		LOG_TRACE("Dynamic Page Size: ", requested_.fileSize);
			return (requested_);
	}
	if (!access(path.c_str(), R_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied, cannout GET file");
	requested_.fileDescriptor->setFd(open(path.c_str(), O_RDONLY | O_NONBLOCK));
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(path));
	return (std::move(requested_));
}

t_file MethodHandler::callPostMethod(std::filesystem::path &path, t_server_config server, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody)
{
	LOG_TRACE("Calling POST: ", path);
	(void)server;
	(void)requestLine;//TODO remove the parameters if unneeded
	if (!access(path.c_str(), W_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied, cannot POST file");
	if (requestHeader.find("multipart/form") == requestHeader.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Requet, Multipart/Form Not Found");
	if (requestBody.find("content-type") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, NO Content Type");
	setContentLength(requestBody);
	checkContentType(requestBody);
	std::filesystem::path filename = createPostFilename(path, requestBody);
	requested_.fileDescriptor->setFd(open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644));
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(filename));
	return (std::move(requested_));
}

std::filesystem::path MethodHandler::createPostFilename(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestBody)
{
	std::filesystem::path result = path;
	if (requestBody.find("filename") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Filename not found");
	result += createFileName(requestBody["filename"]);
	return (result);
}

// Only throw if something is wrong, otherwise success is assumed
void MethodHandler::callDeleteMethod(std::filesystem::path &path)
{
	LOG_TRACE("Calling DELETE: ", path);
	if (!access(path.c_str(), X_OK))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Permission denied to file");
	if (!std::filesystem::remove(path))
		throw WebServErr::SysCallErrException("Failed to delete selected file");
}

t_file MethodHandler::callCGIMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestLine, std::unordered_map<std::string, std::string> requestHeader, std::unordered_map<std::string, std::string> requestBody, EpollHelper &epoll_helper)
{
	LOG_TRACE("Calling CGI: ", path);
	CGIHandler cgi(epoll_helper);
	requested_ = cgi.getCGIOutput(path, requestLine, requestHeader, requestBody);
	return (std::move(requested_));
}

void MethodHandler::setContentLength(std::unordered_map<std::string, std::string> requestBody)
{
	LOG_TRACE("Setting Content Length", "... let's see how big this sucker gets");
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
	LOG_TRACE("Checking content type", "... again");
	if (requestBody.find("disposition-type") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, No Disposition Type");
	if (requestBody["disposition-type"] != "form-data")
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Wrong Disposition Type");
	if (requestBody.find("name") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, No Name Key");
	if (requestBody["name"] == "" || requestBody["name"].empty())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Name Is Empty String");
	if (requestBody.find("Content-Type") == requestBody.end())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, No Content Type");
	if (requestBody["Content-Type"] != "image/png" || requestBody["Content-Type"].empty())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Wrong Content Type");
}

void MethodHandler::checkIfRegFile(const std::filesystem::path &path)
{
	LOG_TRACE("Checking if this is a regular file: ", path);
	if (!std::filesystem::is_regular_file(path))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File is not a regular file");
}

void MethodHandler::checkIfSymlink(const std::string &path)
{
	LOG_TRACE("Checking if this is a symlink: ", path);
	if (std::filesystem::is_symlink(path))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File or Directory is a symlink");
}

bool MethodHandler::checkIfDirectory(std::unordered_map<std::string, t_location_config> &locations, const std::filesystem::path &path)
{
	LOG_TRACE("Checking if this is a directory: ", path);
	if (std::filesystem::is_directory(path))
	{
		if (path.string().back() != '/')
			throw WebServErr::MethodException(ERR_301_REDIRECT, "Location Moved");
		if (locations.contains(path))
		{
			std::string tempDir = path.string() + locations[path].index;
			if (!std::filesystem::exists(tempDir))
				return (true);
			return (false);
		}
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Target is a directory");
	}
	return (false);
}

void MethodHandler::checkIfLocExists(const std::filesystem::path &path)
{
	LOG_TRACE("Checking if this location exists: ", path);
/*	std::error_code ec;

	std::cout << "---- Diagnostics ----\n";
	std::cout << "Input path:      " << path << "\n";
	std::cout << "Absolute path:   " << std::filesystem::absolute(path, ec) << "\n";
	std::cout << "Current dir:     " << std::filesystem::current_path() << "\n";

	bool ex = std::filesystem::exists(path, ec);
	std::cout << "Exists?          " << (ex ? "YES" : "NO");
	if (ec) std::cout << " (error: " << ec.message() << ")";
	std::cout << "\n";

	if (ex) {
		std::cout << "Is regular file? " << (std::filesystem::is_regular_file(path) ? "YES" : "NO") << "\n";
		std::cout << "Is directory?    " << (std::filesystem::is_directory(path) ? "YES" : "NO") << "\n";
		std::cout << "Is symlink?      " << (std::filesystem::is_symlink(path) ? "YES" : "NO") << "\n";
	}

	std::cout << "--------------------\n";
*/	if (!std::filesystem::exists(path))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Location does not exist");
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
	LOG_TRACE("Creating the File name for: ", path);
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
	LOG_TRACE("Creating real path for: ", target);
	LOG_TRACE("Server Root: ", server);
	std::filesystem::path root = std::filesystem::weakly_canonical(server);
	LOG_TRACE("Root: ", root);
	std::filesystem::path targetPath = std::filesystem::path(target).lexically_normal();
	LOG_TRACE("Sought: ", targetPath);
	if (targetPath.is_absolute())
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Target path must be relative");
	std::filesystem::path combined = root / targetPath;
	std::filesystem::path canonicalCombined;
	try
	{
		canonicalCombined = std::filesystem::canonical(combined);
	}
	catch (const std::filesystem::filesystem_error &e)
	{
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Target path does not exist");
	}

	LOG_TRACE("Canonical Root: ", root);
	LOG_TRACE("Canonical Combined: ", canonicalCombined);
	if (canonicalCombined.string().compare(0, root.string().size(), root.string()) != 0 || (canonicalCombined.string().size() > root.string().size() && canonicalCombined.string()[root.string().size()] != std::filesystem::path::preferred_separator))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Security Warning: Path escapes server root");
	return (canonicalCombined);
}

std::string MethodHandler::generateDynamicPage(std::filesystem::path &path)
{
	LOG_TRACE("Dynamically generating page: ", path);
	std:: string page = "<ul>";
	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		page.append("<li>");
		page.append(entry.path());
		page.append("</li>");
	}
	page.append("</ul>");
	std::cout << "Page Size: " << page.size() << std::endl;
	return (page);
}
