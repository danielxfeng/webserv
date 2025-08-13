
// #include "WebServ.hpp"
#include "LogSys.hpp"
#include "HttpRequests.hpp"

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



	std::vector<char> request = {
    'G','E','T',' ','/','f','i','l','e','.','t','x','t',' ','H','T','T','P','/','1','.','0','\r','\n',
    'H','o','s','t',':',' ','l','o','c','a','l','h','o','s','t',':','8','0','8','0','\r','\n',
    'C','o','n','n','e','c','t','i','o','n',':',' ','k','e','e','p','-','a','l','i','v','e','\r','\n',
    'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','i','m','a','g','e','/','p','n','g','\r','\n',
    'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',':',' ','g','z','i','p',',',' ','d','e','f','l','a','t','e','\r','\n',
    'A','c','c','e','p','t','-','L','a','n','g','u','a','g','e',':',' ','e','n','-','U','S',',','e','n',';','q','=','0','.','9','\r','\n',
    '\r','\n','{','t','e','s','t','}'
};




	
	HttpRequests parser;
	parser.httpParser(request);
	return (0);
}
