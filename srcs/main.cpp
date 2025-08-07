
#include "WebServ.hpp"
#include "LogSys.hpp"

int main(/*int argc, char **argv*/)
{
/*	if (argc != 2)
	{
		//TODO error and log
		return (1);
	}
*/
	std::string	test = "Testing this string,";
	LOG_TRACE("Trace", test, __FUNCTION__);
	LOG_DEBUG("Debug", test);
	LOG_INFO("Info", test);
	LOG_WARN("Warning", test);
	LOG_ERROR("Error", test);
	LOG_FATAL("Fatal", test);
	return (0);
}
