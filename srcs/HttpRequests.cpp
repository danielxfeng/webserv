#include "../includes/HttpRequests.hpp"

/*
the request has 3 parts,
request line: is always same structure, the method, target, http version.
			there are 2 spaces seperating them always, and extra space is not correct.
			so it will bad request line if there are extra spaces.
			ex: <method> <path> <version>
request header: always same formate also like <Header-Name>: <Header-Value>
request body: start after an empty line at the end of the header, then
				and ex: {"name":"John","age":30}, and it must match the content type in the header.
(RFC 7230) is strict for more details.
				*/



				/*
				when method is GET:
				for requestHeader; the mandatory parts are 
					Must is:Host.
					Optional are: User-Agent, Accept,Connection.

				when Method is post:
				Must be Content-Length, Body,  
				*/
HttpRequests::HttpRequests():requestString(""){
	
};
		// HttpRequests::HttpRequests(const HttpRequests &obj){};
		// HttpRequests HttpRequests::operator=(const HttpRequests &obj){};
HttpRequests::~HttpRequests(){};


bool HttpRequests::extractRequestBody(size_t &i, size_t requestLength, const std::vector<char> &request){
	for (; i <= requestLength; i++)
		{
			requestBody+=request[i];
		}
	return (true);
	
}

bool HttpRequests::extractRequestLine(size_t &i, size_t requestLength, const std::vector<char> &request){
	std::string method;
	std::string target;
	std::string httpVersion;

	for (; i <= requestLength; i++)
		{
			if(request[i] == '\r' &&  request[i+1] == '\n' )
			{
				i = i+2;
				break;
			}
			requestLine+=request[i];
		}


	size_t j = 0;
	for(; j<requestLine.size();j++)
	{
		if(requestLine[j] == ' ')
		{
			j+=1;
			break;
		}
		method += requestLine[j];
	}
	for(; j<requestLine.size();j++)
	{
		if(requestLine[j] == ' ')
		{
			j+=1;
			break;
		}
		target += requestLine[j];
	}

	for(; j<requestLine.size();j++)
	{
		httpVersion += requestLine[j];
	}

	requestLineMap["Method"] = method;
	requestLineMap["Target"] = target;
	requestLineMap["HttpVersion"] = httpVersion;
	return (true);
}


bool HttpRequests::extractRequestHeader(size_t &i, size_t requestLength, const std::vector<char> &request){

	std::string firstPart;
	std::string secondPart;
	bool secondPartBool = false;

	for (; i <= requestLength; i++)
			{
				if(request[i] == '\r' && request[i+1]&&  request[i+1] == '\n' &&request[i+2] && request[i+2] == '\r' && request[i+3] && request[i+3] == '\n')
				{
					i+=4;
					break;
				}
				requestHeader+=request[i];
			}
	size_t j = 0;

	for (; j< requestHeader.length(); j++)
	{
		if(requestHeader[j] == '\r' && requestHeader[j+1]&& requestHeader[j+1] == '\n' ){
			j+=2;
			secondPartBool = false;
			requestHeaderMap[firstPart] = secondPart;
		}

		if(requestHeader[j] == ':' && requestHeader[j + 1]  && requestHeader[j + 1] == ' '){
			secondPartBool = true;
			j += 2;
		}
			
		if(!secondPartBool)
			firstPart+=requestHeader[j];
		if (secondPartBool)
			secondPart += requestHeader[j];
	}

			
	
	return (true);
}


bool HttpRequests::validateRequestHeader(){
	
	for(const auto& pair:requestHeaderMap)		std::cout<<pair.first<<":"<<pair.second<<std::endl;

	return (true);
}



bool HttpRequests::validateHttpVersion()
{
	if( requestLineMap["HttpVersion"] == "HTTP/1.1" || requestLineMap["HttpVersion"] == "HTTP/1.0")
		return (true);
	return false;
}

bool HttpRequests::validateTarget()
{
	if(requestLineMap["Target"].empty())
		throw std::runtime_error("400 Bad Request: Invalid request line");
	std::string invalidCharactersUri = " <>\"{}|\\^`";
	for(char j:requestLineMap["Target"] ){
		for(char i:invalidCharactersUri)
			{
				if (i == j)
					throw std::runtime_error("400 Bad Request: Invalid request line");
			}
	}
	return true;
}


bool HttpRequests::validateMethod()
{
	std::vector<std::string> validMethods = {"GET", "POST", "DELETE"};

	for(std::string im:validMethods)
	{
		if(requestLineMap["Method"] == im)
			return (true);
	}
	throw std::runtime_error("400 Bad Request: Invalid request line");
	return (false);
}

bool HttpRequests::validateRequestLine(){

	//validate the method type
	if(!validateMethod())
		return (false);
	if (!validateTarget())
		return false;
	if(!validateHttpVersion())
		return false;
	return (true);
}

HttpRequests &HttpRequests::httpParser(const std::vector<char> &request){

	size_t requestLength = request.size();
	size_t i = 0;
	
	if(!extractRequestLine(i, requestLength, request))
		std::cerr<<"extractRequestLine";

	if(!validateRequestLine())
		std::cerr<<"validateRequestLine";		

	if(!extractRequestHeader(i, requestLength, request))
			std::cerr<<"extractRequestHeader";

	if(!validateRequestHeader())
			std::cerr<<"Validate extractRequestHeader";
	
	if (i != requestLength)
		if(!extractRequestBody(i, requestLength, request))
			std::cerr<<"extractRequestBody";

	// std::cout<<"Request Line:\n"<<requestLine<<"\n";
	// std::cout<<"\n";
	// std::cout<<"Request Header:\n"<<requestHeader<<"\n";
	// std::cout<<"\n";
	// std::cout<<"Request Body:\n"<<requestBody<<"\n";
	// std::cout<<"\n";

	// std::cout<<requestLineMap["Method"];

return(*this);

}