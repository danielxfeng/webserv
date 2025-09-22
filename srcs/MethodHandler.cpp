#include "../includes/MethodHandler.hpp"
#include <algorithm>
#include <filesystem>

MethodHandler::MethodHandler(EpollHelper &epoll_helper)
{
	requested_.FD_handler_IN = std::make_shared<RaiiFd>(epoll_helper);
	requested_.FD_handler_OUT = std::make_shared<RaiiFd>(epoll_helper);
	requested_.expectedSize = 0;
	requested_.fileSize = 0;
	requested_.isDynamic = false;
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
	if (requestLine.contains("Method"))
	{
		chosenMethod = requestLine["Method"];
		LOG_TRACE("Method found: ", chosenMethod);
	}
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Http Method does not exist");
	std::string rootDestination = matchLocation(server.locations, targetRef);// Find best matching location
	t_method realMethod = convertMethod(chosenMethod);
	for (size_t i = 0; i < server.locations[rootDestination].methods.size(); i++)
	{
		std::cout << "Server methods: " << server.locations[rootDestination].methods[i] << std::endl;
		if (server.locations[rootDestination].methods[i] != realMethod)
			throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Method not allowed or is unknown");
	}
	std::filesystem::path realPath = createRealPath(rootDestination, targetRef);
	checkIfLocExists(realPath);
	bool useAutoIndex = checkIfDirectory(server.locations, realPath);//TODO do we need a check for auto-index or can we just do it automatically?
	if (!useAutoIndex)
		checkIfRegFile(realPath);
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

std::string	MethodHandler::matchLocation(std::unordered_map<std::string, t_location_config> &locations, std::string &targetRef)
{
	LOG_TRACE("Matching Location for: ", targetRef);
	std::string bestMatch = "";
	size_t	longestMatchLength = 0;
	std::string normalizedTargetPath = targetRef;
	if (!normalizedTargetPath.empty() && normalizedTargetPath.back() != '/')
			normalizedTargetPath += '/';
	for (auto it = locations.begin();  it != locations.end(); it++)
	{
		std::string normalizedLocPath = it->first;
		if (!normalizedLocPath.empty() && normalizedLocPath.back() != '/')
			normalizedLocPath += '/';
		if (normalizedTargetPath.find(normalizedLocPath) == 0)
		{
			if (normalizedLocPath.length() > longestMatchLength)
			{
				bestMatch = it->first;
				longestMatchLength = normalizedLocPath.length();
			}
		}
	}
	if (bestMatch.empty() && locations.count("/") > 0)
		bestMatch = "/";
	LOG_TRACE("Best Location Match: ", locations[bestMatch].root);
	return (locations[bestMatch].root);
}

t_file MethodHandler::callGetMethod(bool useAutoIndex, std::filesystem::path &path)
{
	LOG_TRACE("Calling GET: ", path);
	if (useAutoIndex)
	{
		LOG_TRACE("Directory is: ", "auto-index");
		requested_.dynamicPage = generateDynamicPage(path);
		requested_.isDynamic = true;
		requested_.fileSize = requested_.dynamicPage.size();
		LOG_TRACE("Dynamic Page Size: ", requested_.fileSize);
			return (requested_);
	}
	LOG_WARN("Path.c_str: ", path.c_str());
	requested_.FD_handler_OUT->setFd(open(path.c_str(), O_RDONLY | O_NONBLOCK));	
	if (requested_.FD_handler_OUT.get()->get() == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Permission denied, cannout GET file");
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
	requested_.FD_handler_OUT->setFd(open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644));
	if (requested_.FD_handler_OUT.get()->get() == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Permission denied, cannout POST file");
	LOG_TRACE("GET FD: ", requested_.FD_handler_OUT->get());
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

bool MethodHandler::checkIfDirectory(std::unordered_map<std::string, t_location_config> &locations, std::filesystem::path &path)
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
			path = std::filesystem::path(tempDir);
			return (false);
		}
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Target is a directory");
	}
	return (false);
}

void MethodHandler::checkIfLocExists(const std::filesystem::path &path)
{
	LOG_TRACE("Checking if this location exists: ", path);
	if (!std::filesystem::exists(path))
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
std::vector<std::filesystem::path> MethodHandler::splitPath(const std::filesystem::path &path)
{
    std::vector<std::filesystem::path> parts;
    for (const auto& part : path) {
        if (part != "/" && part != "")
            parts.push_back(part);
    }
    return (parts);
}

std::string	MethodHandler::stripLocation(const std::string &server, const std::string &target)
{
	LOG_TRACE("Stripping Location: ", server);
	auto norm_target = std::filesystem::weakly_canonical(target);
	auto norm_location = std::filesystem::weakly_canonical(server);
	auto target_parts = splitPath(norm_target);
	auto location_parts = splitPath(norm_location);
	//Check if target is empty 
	if (target_parts.empty())
		return ("/");
	//Checks if target is at root
	if (target_parts.size() == 1)
		return (target_parts[0].string());
	std::filesystem::path result = "";
	//Finds which parts of target overlaps with location
	for (ssize_t i = static_cast<ssize_t>(location_parts.size() - 1); i != -1 ; i--)
	{
		for (ssize_t k = static_cast<ssize_t>(target_parts.size() - 1); k != -1; k--)
		{
			if (target_parts[k] != location_parts[i])
				continue ;
			else if (target_parts[k] == location_parts[i])
			{
				for (size_t x = k + 1; x < target_parts.size(); x++)
					result = result / target_parts[x];
				break;
			}
		}
	}
	// if (result == "")
	// 	result = norm_target;
    return (result.string());
}

std::filesystem::path MethodHandler::createRealPath(const std::string &server, const std::string &target)
{
	LOG_TRACE("Creating real path for: ", target);
	std::filesystem::path targetPath(stripLocation(server, target));
	if (targetPath == "")
	{
		LOG_TRACE("Strip Location returned: ", "Empty");
		targetPath = std::filesystem::path(target);
		targetPath = std::filesystem::weakly_canonical(targetPath);
	}
	LOG_TRACE("Relative Path: ", targetPath);
	std::filesystem::path root(server);
//	root = std::filesystem::weakly_canonical(root);
	LOG_TRACE("Root Path: ", root);
	std::filesystem::path combinedPath = root / targetPath;//std::filesystem::path(root.string() + targetPath.string());
	LOG_TRACE("Checking if this is a symlink: ", combinedPath);
	if (std::filesystem::is_symlink(combinedPath))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File or Directory is a symlink");
	 std::filesystem::path canonical = std::filesystem::weakly_canonical(combinedPath);
	return (canonical);
}

std::string MethodHandler::generateDynamicPage(std::filesystem::path &path)//TODO Fix generator to return links and only within the provided directory
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
