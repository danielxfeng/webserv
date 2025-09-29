#include "../includes/MethodHandler.hpp"
#include <algorithm>
#include <cstddef>
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

	if (requestLine.contains("Target"))
	{
		targetRef = requestLine["Target"];
		LOG_TRACE("Target found: ", targetRef);
	}
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Http Target does not exist");
	std::string chosenMethod;
	std::cout << requestLine["Method"] << std::endl;

	if (!requestLine.contains("Method"))
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Http Method does not exist");
	chosenMethod = requestLine["Method"];
	LOG_TRACE("Method found: ", chosenMethod);

	std::string rootDestination = matchLocation(server.locations, targetRef); // Find best matching location
	std::string root = server.locations[rootDestination].root;
	LOG_DEBUG("rootDestination: ", rootDestination);
	LOG_DEBUG("Root: ", root);
	t_method realMethod = convertMethod(chosenMethod);
	LOG_DEBUG("Real Method: ", realMethod);
	if (std::find(server.locations[rootDestination].methods.begin(), server.locations[rootDestination].methods.end(), realMethod) == server.locations[rootDestination].methods.end())
		throw WebServErr::MethodException(ERR_405_METHOD_NOT_ALLOWED, "Method not allowed or is unknown");

	// Clean target - removing overlap with root
	std::string path = stripLocation(rootDestination, targetRef);

	// TODO maybe check here if there's an index
	if (path == "/" && !server.locations[rootDestination].index.empty())
		path += server.locations[rootDestination].index;

	// Create realPath
	std::filesystem::path realPath(root + path);
	LOG_DEBUG("Real Path: ", realPath);

	// Check if location exists
	checkIfLocExists(realPath);

	// Make canonical
	std::filesystem::path canonical = std::filesystem::weakly_canonical(realPath);
	LOG_DEBUG("Canonical Path: ", canonical);

	// Check if Symlink
	if (std::filesystem::is_symlink(canonical))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Destination is a symlink");

	// Check if safe
	if (!checkIfSafe(rootDestination, canonical))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Path goes beyond root");

	// Check if Directory
	bool useAutoIndex = checkIfDirectory(server.locations, canonical, rootDestination, realPath);
	if (!useAutoIndex)
		checkIfRegFile(canonical);

	switch (realMethod)
	{
	case GET:
		return (callGetMethod(useAutoIndex, canonical));
	case POST:
		return (callPostMethod(canonical, requestHeader, targetRef, root));
	case DELETE:
	{
		callDeleteMethod(canonical);
		return (requested_);
	}
	case CGI:
		return (callCGIMethod(canonical, requestLine, requestHeader, requestBody, epoll_helper));
	default:
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Method not allowed or is unknown");
	}
}

std::string MethodHandler::matchLocation(std::unordered_map<std::string, t_location_config> &locations, std::string &targetRef)
{
	LOG_TRACE("Matching Location for: ", targetRef);
	std::string bestMatch = "";
	size_t longestMatchLength = 0;
	std::string normalizedTargetPath = targetRef;
	if (!normalizedTargetPath.empty() && normalizedTargetPath.back() != '/')
		normalizedTargetPath += '/';
	for (auto it = locations.begin(); it != locations.end(); it++)
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
	LOG_TRACE("Best Location Match: ", bestMatch, " Root: ", locations[bestMatch].root);
	return (bestMatch);
}

t_file MethodHandler::callGetMethod(bool useAutoIndex, std::filesystem::path &path)
{
	LOG_TRACE("Calling GET: ", path);
	if (useAutoIndex)
	{
		LOG_TRACE("Directory is: ", "auto-index");
		try
		{
			requested_.dynamicPage = generateDynamicPage(path);
			requested_.isDynamic = true;
			requested_.fileSize = requested_.dynamicPage.size();
			LOG_TRACE("Dynamic Page Size: ", requested_.fileSize);
		}
		catch (...)
		{
			throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Auto-index failed");
		}
		return (requested_);
	}
	LOG_WARN("Path.c_str: ", path.c_str());
	if (access(path.string().c_str(), R_OK) == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Permission denied, cannout GET file");
	requested_.FD_handler_OUT->setFd(open(path.c_str(), O_RDONLY | O_NONBLOCK));
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(path));
	return (std::move(requested_));
}

t_file MethodHandler::callPostMethod(std::filesystem::path &path, std::unordered_map<std::string, std::string> requestHeader, std::string & targetRef, const std::string &root)
{
	LOG_TRACE("Calling POST: ", path);
	if (checkFileCount(root) > 25)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Too Many Files, Delete Some");
	if (requestHeader.contains("multipart/form"))
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Requet, Multipart/Form Not Found");
	std::string extension;
	std::string fileType = requestHeader["content-type"];
	if (fileType == "image/png")
		extension = ".png";
	else if (fileType == "image/jpeg" || fileType == "image/jpg")
		extension = ".jpg";
	else if (fileType == "text/plain")
		extension = ".txt";
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Wrong File Type");
	std::filesystem::path filename = createRandomFilename(path, extension);
	std::string result = targetRef + '/' + filename.filename().string();
	LOG_DEBUG("postFilename: ", result);
	requested_.postFilename = result;
	requested_.FD_handler_OUT->setFd(open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0644));
	if (requested_.FD_handler_OUT.get()->get() == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Permission denied, cannout POST file");
	LOG_TRACE("GET FD: ", requested_.FD_handler_OUT->get());
	requested_.fileSize = static_cast<int>(std::filesystem::file_size(filename));
	return (std::move(requested_));
}

std::filesystem::path MethodHandler::createRandomFilename(std::filesystem::path &path, std::string &extension)
{
	std::string result = path.string();
	if (result.back() != '/')
		result.push_back('/');
	result += "upload_";
	static const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static std::mt19937 rng{static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count())};
	std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

	for (size_t i = 0; i < 12; i++)
		result.push_back(chars[dist(rng)]);
	result += extension;
	std::filesystem::path uploadCheck(result);
	if (std::filesystem::exists(uploadCheck))
		createRandomFilename(path, extension);
	return (uploadCheck);
}

// Only throw if something is wrong, otherwise success is assumed
void MethodHandler::callDeleteMethod(std::filesystem::path &path)
{
	LOG_TRACE("Calling DELETE: ", path);
	if (access(path.c_str(), W_OK) == -1)
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Permission Denied: cannot delete selected file");
	if (std::filesystem::is_directory(path))
		throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Target is a directory, cannot DELETE");
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

void MethodHandler::setContentLength(std::unordered_map<std::string, std::string> requestHeader)
{
	LOG_TRACE("Setting Content Length", "... let's see how big this sucker gets");
	if (!requestHeader.contains("content-length"))
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Failed to get content length."); // TODO double check error code
	std::stringstream length(requestHeader["content-length"]);
	length >> requested_.expectedSize;
	if (length.fail())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, content length not found."); // TODO double check error code
	if (!length.eof())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, failed to get end of file for content length."); // TODO double check error code
																															  // if (requested_.expectedSize > MAX_BODY_SIZE)
																															  //	throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "Body size too large."); // TODO double check error code
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
	if (requestBody["Content-Type"] == "application/octet-stream" || requestBody["Content-Type"].empty())
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Bad Request, Wrong Content Type");
}

void MethodHandler::checkIfRegFile(const std::filesystem::path &path)
{
	LOG_TRACE("Checking if this is a regular file: ", path);
	if (!std::filesystem::is_regular_file(path))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File is not a regular file");
}

bool MethodHandler::checkIfDirectory(std::unordered_map<std::string, t_location_config> &locations, std::filesystem::path &path, const std::string &rootDestination, const std::filesystem::path &realPath)
{
	LOG_TRACE("Checking if this is a directory: ", path);
	if (!std::filesystem::is_directory(path))
		return (false);
	LOG_DEBUG("realPath: ", realPath);
	LOG_DEBUG("Canonical: ", path);
	if (!realPath.empty() && realPath.string().back() != '/' && realPath.string().back() != std::filesystem::path::preferred_separator)
		throw WebServErr::MethodException(ERR_301_REDIRECT, "Location Moved");
	LOG_TRACE("This is a directory: ", path);
	if (locations.contains(rootDestination))
	{
		std::filesystem::path tempDir(rootDestination);
		if (locations.contains(path))
		{
			tempDir /= std::filesystem::path(locations[path].index);
			LOG_TRACE("Attempting to add index.html: ", tempDir);
			if (!std::filesystem::exists(tempDir))
			{
				LOG_TRACE("Auto Index set to: ", "ON");
				return (true);
			}
			path = std::filesystem::path(tempDir);
			LOG_TRACE("Auto Index set to: ", "OFF");
			return (false);
		}
		else
		{
			LOG_TRACE("Auto Index set to: ", "ON");
			return (true);
		}
	}
	throw WebServErr::MethodException(ERR_403_FORBIDDEN, "Target is a forbidden directory");
}

void MethodHandler::checkIfLocExists(const std::filesystem::path &path)
{
	LOG_TRACE("Checking if this location exists: ", path);
	if (!std::filesystem::exists(path))
		throw WebServErr::MethodException(ERR_404_NOT_FOUND, "Location does not exist");
}

std::string MethodHandler::stripLocation(const std::string &rootDestination, const std::string &targetRef)
{
	std::string path = targetRef;
	LOG_TRACE("Target to be cleaned: ", path);
	std::string toRemove = rootDestination;
	size_t pos = path.find(toRemove);
	LOG_DEBUG("Size of pos: ", pos);
	if (pos != std::string::npos)
		path.erase(pos, toRemove.length());
	if (path.empty())
		path = "/";
	else if (path.front() != '/')
		path = '/' + path;
	LOG_TRACE("Cleaned target: ", path);
	return (path);
}

std::filesystem::path MethodHandler::createRealPath(const std::string &root, const std::string &target)
{
	LOG_TRACE("Creating real path for: ", target);
	std::filesystem::path targetPath(target);

	LOG_TRACE("Weakly Canonical Target Path: ", targetPath);
	std::filesystem::path prefix(root);
	LOG_TRACE("Root Path: ", prefix);
	std::filesystem::path combinedPath;
	if (targetPath == "/")
		combinedPath = root + '/';
	else
		combinedPath = prefix / targetPath;
	LOG_TRACE("Checking if this is a symlink: ", combinedPath);
	if (std::filesystem::is_symlink(combinedPath))
		throw WebServErr::MethodException(ERR_500_INTERNAL_SERVER_ERROR, "File or Directory is a symlink");
	std::filesystem::path canonical = std::filesystem::weakly_canonical(combinedPath);
	return (canonical);
}

std::string MethodHandler::generateDynamicPage(std::filesystem::path &path)
{
	std::string base = path.string();
	if (base.empty() || base[0] != '/')
		base = '/' + base;
	std::filesystem::path fullPath(base);
	LOG_TRACE("Dynamically generating page for: ", fullPath);
	std::string page = "<!DOCTYPE html>\n<html>\n<head><title>Directory Structure of " + path.string() + "</title></head>\n";
	page += "<body>\n<h1>" + path.string() + "</h1>\n<ul>\n";

	for (const auto &entry : std::filesystem::directory_iterator(fullPath))
	{
		std::string name = entry.path().filename().string();
		if (std::filesystem::is_directory(name))
			name += '/';
		if (!path.empty() && path.string().back() != '/')
			name += '/';
		std::string link = "<a href=\"" + name + "\">" + name + "</a>";
		page.append("<li>" + link + "</li>");
	}

	page.append("</ul>\n</body>\n</html>\n");
	std::cout << "Page Size: " << page.size() << std::endl;
	return (page);
}

bool MethodHandler::checkIfSafe(const std::filesystem::path &root, const std::filesystem::path &path)
{
	LOG_TRACE("Checking if path is safe: ", path, " Root is: ", root);
	try
	{
		std::filesystem::path fullPath = std::filesystem::weakly_canonical(root / path.relative_path());

		return (std::mismatch(root.begin(), root.end(), fullPath.begin()).first == root.end());
	}
	catch (const std::filesystem::filesystem_error &e)
	{
		LOG_TRACE("Forbidden Path: ", e.what());
		return (false);
	}
}

size_t MethodHandler::checkFileCount(const std::string &root)
{
	size_t fileCount = 0;
	LOG_TRACE("Directory File Count for: ", root);
	for (const auto &entry : std::filesystem::directory_iterator(root))
	{
		if (std::filesystem::is_regular_file(entry.path()))
			fileCount++;
	}
	LOG_TRACE("Directory File Count: ", fileCount);
	return (fileCount);
}
