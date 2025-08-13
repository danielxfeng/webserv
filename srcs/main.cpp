
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
    'P','O','S','T',' ','/','f','i','l','e','.','t','x','t',' ','H','T','T','P','/','1','.','1','\r','\n',
    'H','o','s','t',':',' ','l','o','c','a','l','h','o','s','t',':','8','0','8','0','\r','\n',
    'C','o','n','n','e','c','t','i','o','n',':',' ','k','e','e','p','-','a','l','i','v','e','\r','\n',
    'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','m','u','l','t','i','p','a','r','t','/','f','o','r','m','-','d','a','t','a',';',' ','b','o','u','n','d','a','r','y','=','-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','\r','\n',
    'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','8','8','\r','\n','\r','\n',
    '-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','\r','\n',
    'C','o','n','t','e','n','t','-','D','i','s','p','o','s','i','t','i','o','n',':',' ','f','o','r','m','-','d','a','t','a',';',' ','n','a','m','e','=','"','t','e','s','t','"','\r','\n',
    '\r','\n',
    '{','t','e','s','t','}','\r','\n',
    '-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','-','-','\r','\n'
};

	HttpRequests parser;
	parser.httpParser(request);
	return (0);
}
