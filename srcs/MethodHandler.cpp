#include "../includes/MethodHandler.hpp"

MethodHandler::MethodHandler()
{}

MethodHandler::MethodHandler(const MethodHandler &copy)
{
    *this = copy;
}

MethodHandler::~MethodHandler()
{}

MethodHandler &MethodHandler::operator=(const MethodHandler &copy)
{
    if (this != &copy)
    {

    }
    return (*this);
}

int		MethodHandler::handleMethod(t_method method, std::unordered_map<std::string, std::string> headers)
{
	switch (method)
	{
		case GET:
			LOG_TRACE("Calling GET ", headers);
			return (callGetMethod(headers));
		case POST:
			LOG_TRACE("Calling POST ", headers);
			return (callPostMethod(headers));
		case DELETE:
			LOG_TRACE("Calling DELETE ", headers);
			callDeleteMethod(headers);
			return (0);
		case CGI:
			LOG_TRACE("Calling CGI: ", headers);
			return (callCGIMethod(headers));
		default:
			LOG_WARN("Method is unknown: ", headers);
			throw WebServErr::MethodException("Unknown Method");
	}
}

int		MethodHandler::callGetMethod(std::unordered_map<std::string, std::string> headers)
{}

int		MethodHandler::callPostMethod(std::unordered_map<std::string, std::string> headers)
{}

//Only throw if something is wrong, otherwise success is assumed
void	MethodHandler::callDeleteMethod(std::unordered_map<std::string, std::string> headers)
{}

int		MethodHandler::callCGIMethod(std::unordered_map<std::string, std::string> headers)
{}
