#include "RedirectHandler.hpp"
RedirectHandler::RedirectHandler() {}

RedirectHandler::~RedirectHandler() {}

void	RedirectHandler::checkRedirection(t_server_config &server, std::unordered_map<std::string, std::string> &requestLine)
{
	std::string targetRef;
	if (requestLine.contains("Target"))
	{
		targetRef = requestLine["Target"];
		LOG_TRACE("Target found: ", targetRef);
	}
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Redirect Handler: Http Target does not exist");
	std::string rootDestination = matchLocation(server.locations, targetRef); // Find best matching location
	std::string root = server.locations[rootDestination].root;
	LOG_DEBUG("rootDestination: ", rootDestination);
	LOG_DEBUG("Root: ", root);
	
	//Check methods, if not redirect return else throw 301 with root
	if (!requestLine.contains("Method"))
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Redirect Handler: Http Method does not exist");
	if (std::find(server.locations[rootDestination].methods.begin(), server.locations[rootDestination].methods.end(), REDIRECT) == server.locations[rootDestination].methods.end())
		return ;
	throw WebServErr::MethodException(ERR_301_REDIRECT, root);
}

std::string RedirectHandler::matchLocation(const std::unordered_map<std::string, t_location_config> &locations, const std::string &targetRef)
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
	std::string root = locations.find(bestMatch)->second.root;
	LOG_TRACE("Best Location Match: ", bestMatch, " Root: ", root);
	return (bestMatch);
}