#include "../includes/CGIHandler.hpp"
#include <unordered_map>
#include <vector>

CGIHandler::CGIHandler()
{}

CGIHandler::CGIHandler(const CGIHandler &copy)
{
    *this = copy;
}

CGIHandler::~CGIHandler()
{}

CGIHandler &CGIHandler::operator=(const CGIHandler &copy)
{
    if (this != &copy)
    {

    }
    return (*this);
}

std::vector<std::string>    CGIHandler::setENVP(std::unordered_map<std::string, std::string> headers)
{
	(void) headers;
	std::vector<std::string> data;
	return (data);
}
