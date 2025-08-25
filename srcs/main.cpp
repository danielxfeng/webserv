
// #include "WebServ.hpp"
#include "LogSys.hpp"
#include "HttpRequests.hpp"
#include "HttpResponse.hpp"



bool validateConfigFile(std::string_view &fileName){
	size_t found;

	found = fileName.find(".json");
	if(found == std::string::npos){
		std::cerr<<"Error, you have to include the config file.";
		return (false);
	}
	if (found != (fileName.size() - 5)){
		std::cerr<<"Error, file name is not correct.";
		return (false);
	}

	return (true);
}


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr<<"Error, you have to include the config file.";
		return (1);
	}
	std::string_view fileName= argv[1];
	if(!validateConfigFile(fileName)){
		return (1);
		
	}


	std::string	test = "Testing this string,";
	LOG_TRACE("Trace", test, __FUNCTION__);
	LOG_DEBUG("Debug", test);
	LOG_INFO("Info", test);
	LOG_WARN("Warning", test);
	LOG_ERROR("Error", test);
	LOG_FATAL("Fatal", test);


// std::vector<char> request = {
//     'P','O','S','T',' ','/','u','p','l','o','a','d',' ','H','T','T','P','/','1','.','1','\r','\n',
//     'H','o','s','t',':',' ','l','o','c','a','l','h','o','s','t',':','8','0','8','0','\r','\n',
//     'C','o','n','n','e','c','t','i','o','n',':',' ','k','e','e','p','-','a','l','i','v','e','\r','\n',
//     'T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',':',' ','c','h','u','n','k','e','d','\r','\n',
//     'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','m','u','l','t','i','p','a','r','t','/','f','o','r','m','-','d','a','t','a',';',' ','b','o','u','n','d','a','r','y','=','-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','\r','\n',
//     '\r','\n',


//     '1','a','\r','\n',
//     '-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','\r','\n','\r','\n',

//     '2','e','\r','\n',
//     'C','o','n','t','e','n','t','-','D','i','s','p','o','s','i','t','i','o','n',':',' ','f','o','r','m','-','d','a','t','a',';',' ','n','a','m','e','=','"','f','i','e','l','d','1','"','\r','\n','\r','\n','v','a','l','u','e','1','\r','\n',


//     '1','9','\r','\n',

//     '-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','-','-','\r','\n','\r','\n',


//     '0','\r','\n',
//     '\r','\n'
// };

// std::vector<char> request = {
//     'P','O','S','T',' ','/','f','i','l','e','.','t','x','t',' ','H','T','T','P','/','1','.','1','\r','\n',
//     'H','o','s','t',':',' ','l','o','c','a','l','h','o','s','t',':','8','0','8','0','\r','\n',
//     'C','o','n','n','e','c','t','i','o','n',':',' ','k','e','e','p','-','a','l','i','v','e','\r','\n',
//     'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','m','u','l','t','i','p','a','r','t','/','f','o','r','m','-','d','a','t','a',';',' ','b','o','u','n','d','a','r','y','=','-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','\r','\n',
//     'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','8','8','\r','\n','\r','\n',
//     '-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','\r','\n',
//     'C','o','n','t','e','n','t','-','D','i','s','p','o','s','i','t','i','o','n',':',' ','f','o','r','m','-','d','a','t','a',';',' ','n','a','m','e','=','"','t','e','s','t','"','\r','\n',
//     '\r','\n',
//     '{','t','e','s','t','}','\r','\n',
//     '-','-','-','-','B','o','u','n','d','a','r','y','1','2','3','-','-','\r','\n'
// };


std::string request="POST /upload HTTP/1.1\r\n"
					"Host: example.com\r\n"
					"User-Agent: MyUploader/1.0\r\n"
					"Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
					"Content-Length: 213\r\n"
					"\r\n"
					"------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
					"Content-Disposition: form-data; name=\"file\"; filename=\"example.png\"\r\n"
					"Content-Type: image/png\r\n"
					"\r\n"
					"\x89""PNG\r\n\x1a\n"  // start of PNG header bytes
					"\x00\x00\x00\x0DIHDR\x00\x00\x00\x10..."  // placeholder for rest of binary data
					"\r\n"
					"------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

// std::vector<char> request = {
//     'G','E','T',' ','/','u','p','l','o','a','d',' ','H','T','T','P','/','1','.','1','\r','\n',
//     'H','o','s','t',':',' ','l','o','c','a','l','h','o','s','t',':','8','0','8','0','\r','\n',
// 	'\r','\n',
// };
	std::string file = "202";
	HttpRequests parser;
	// HttpResponse response;

	parser.httpParser(request);
	// response.successResponse(conn, parser, file );
	return (0);
}
