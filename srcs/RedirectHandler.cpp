#include "RedirectHandler.hpp"
RedirectHandler::RedirectHandler() {}

RedirectHandler::~RedirectHandler() {}

void	RedirectHandler::checkRedirection(t_server_config &server, std::unordered_map<std::string, std::string> &requestLine)
{
	LOG_TRACE("Checking if target is redirected");
	std::string targetRef;
	if (requestLine.contains("Target"))
		targetRef = requestLine["Target"];
	else
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Redirect Handler: Http Target does not exist");
	std::string rootDestination = matchLocation(server.locations, targetRef); 
	std::string root = server.locations[rootDestination].root;
	
	if (!requestLine.contains("Method"))
		throw WebServErr::MethodException(ERR_400_BAD_REQUEST, "Redirect Handler: Http Method does not exist");
	std::vector<t_method> methods = server.locations[rootDestination].methods;
	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] == REDIRECT)
			WebServErr::MethodException(ERR_301_REDIRECT, root);
	}
}