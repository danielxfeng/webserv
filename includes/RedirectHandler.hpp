#pragma once
#include "SharedTypes.hpp"
#include "LogSys.hpp"
#include "WebServErr.hpp"
#include <string>
#include <unordered_map>
#include <algorithm>
#include "urlHelper.hpp"

class RedirectHandler
{
private:
	
public:
	RedirectHandler();
	RedirectHandler(const RedirectHandler &copy) = delete;
	RedirectHandler &operator=(const RedirectHandler &copy) = delete;
	~RedirectHandler();

	void	checkRedirection(t_server_config &server, std::unordered_map<std::string, std::string> &requestLine);
};