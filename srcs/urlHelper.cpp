#include "urlHelper.hpp"

std::string matchLocation(std::unordered_map<std::string, t_location_config> &locations, std::string &targetRef)
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

std::string stripLocation(const std::string &rootDestination, const std::string &targetRef)
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