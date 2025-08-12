#include "../includes/HttpRequests.hpp"
//default
HttpRequests::HttpRequests():upToBodyCounter(0),requestString(""),requestLine(""),requestHeader(""),requestBody(""),requestServerName(""),requestPort(1),requestLineMap(),requestHeaderMap() {
};

HttpRequests::HttpRequests(const HttpRequests &obj){
	if(this != &obj){
		upToBodyCounter = obj.upToBodyCounter;
		requestString= obj.requestString;
		requestLine= obj.requestLine;
		requestHeader= obj.requestHeader;
		requestBody= obj.requestBody;
		requestServerName= obj.requestServerName;
		requestPort= obj.requestPort;
		requestLineMap= obj.requestLineMap;
		requestHeaderMap= obj.requestHeaderMap;
	}
};

HttpRequests HttpRequests::operator=(const HttpRequests &obj){
	if(this != &obj){
		upToBodyCounter = obj.upToBodyCounter;
		requestString= obj.requestString;
		requestLine= obj.requestLine;
		requestHeader= obj.requestHeader;
		requestBody= obj.requestBody;
		requestServerName= obj.requestServerName;
		requestPort= obj.requestPort;
		requestLineMap= obj.requestLineMap;
		requestHeaderMap= obj.requestHeaderMap;
	}
	return (*this);
};

HttpRequests::~HttpRequests(){
	
};

// request line functions
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



void HttpRequests::validateHttpVersion()
{
	if(requestLineMap["HttpVersion"].empty())
		throw WebServErr::BadRequestException("Http version must be 1.1 ot 1.0");
	if( !(requestLineMap["HttpVersion"] == "HTTP/1.1" || requestLineMap["HttpVersion"] == "HTTP/1.0"))	
		throw WebServErr::BadRequestException("Http version must be 1.1 ot 1.0");

}

void HttpRequests::validateTarget()
{
	if(requestLineMap["Target"].empty())
		throw WebServErr::BadRequestException("target cannot be empty");

	std::string invalidCharactersUri = " <>\"{}|\\^`";
	for(char j:requestLineMap["Target"] ){
		for(char i:invalidCharactersUri)
			{
				if (i == j)
					throw WebServErr::BadRequestException("target cannot has invalid characters");
			}
	}
}


void HttpRequests::validateMethod()
{
	bool valid_method = false;
	std::vector<std::string> validMethods = {"GET", "POST", "DELETE"};
	for(std::string im:validMethods)
	{
		if(requestLineMap["Method"] == im)
			valid_method = true;
	}
	if (!valid_method)
		throw WebServErr::BadRequestException("Invalid method only GET, POST and DELETE are allowed");
}

void HttpRequests::validateRequestLine(){

	//validate the method type
	validateMethod();
	validateTarget();
	validateHttpVersion();
}

// request header functions


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
			//to make sure that Host and content-length has only one value, it is not allowed to be duplicated.
			if ((requestHeaderMap.contains("host") && firstPart=="host") || (requestHeaderMap.contains("content-length") && firstPart=="content-length"))
			{
				std::cerr<<"Error: we have host before"<<std::endl;
				break;
			}
			requestHeaderMap[firstPart] = secondPart;
			firstPart = "";
			secondPart = "";
		}

		if(requestHeader[j] == ':' && requestHeader[j + 1]  && requestHeader[j + 1] == ' '){
			secondPartBool = true;
			j += 2;
		}
			
		if(!secondPartBool)
			firstPart += std::tolower(requestHeader[j]);
		if (secondPartBool)
			secondPart += std::tolower(requestHeader[j]);
	}
	return (true);
}


void HttpRequests::host_validator(void){
	std::string host_str;
	std::string firstPart;
	std::string secondPart;
	bool secondPartBool = false;

	if(!requestHeaderMap.contains("host"))
		throw WebServErr::BadRequestException("host is required");
	host_str = requestHeaderMap["host"];
	for(size_t i = 0; i < host_str.length();i++)
		{
			if (host_str[i] == ':'){
				secondPartBool = true;
				i++;
		}
			if(!secondPartBool)
				firstPart += host_str[i] ;
			else
				secondPart +=host_str[i] ;
		}
		requestServerName = firstPart;
		// TODO check if the ServerName is valid from the list.
		// validate Port
		requestPort = std::stoi(secondPart);
		if (requestPort < 1 || requestPort > 65535)	
			throw WebServErr::BadRequestException("post is out of allowed range from 1 to 655535");

} 


void HttpRequests::content_length_validator(void){
	size_t content_length_var = 0;
	if(requestLineMap["Method"] == "POST" || requestLineMap["Method"] == "DELETE")
	{
		if( !requestHeaderMap.contains("content-length") || requestHeaderMap["content-length"].empty())
			throw WebServErr::BadRequestException("POST and DELETE must have content-length value");
		content_length_var = static_cast<size_t> (stoull(requestHeaderMap["content-length"]));
		if (content_length_var > 1073741824)
			throw WebServErr::BadRequestException("Bad content-length less than 1GB are allowed");
	}
	else if(requestLineMap["Method"] == "GET")
		if( requestHeaderMap.contains("content-length"))
			throw WebServErr::BadRequestException("GET must have no content-length");
} 

void HttpRequests::header_connection_validator(void){
	if (requestHeaderMap.contains("connection")){
		if(!(requestHeaderMap["connection"] == "keep-alive" || requestHeaderMap["connection"] == "close"))
			throw WebServErr::BadRequestException("Incrorrect connection value, must be keep-alive or close");
	}
	else{
		requestHeaderMap["connection"] = "keep-alive";
	}
		
}


std::vector<std::string> HttpRequests::stov(std::string &string){
	std::vector<std::string> result;
	std::string tmp;
	for(size_t i=0;i < string.length(); i++){
		if(string[i] == ','){
			result.push_back(tmp);
			tmp="";
			i++;
		}
		tmp+=string[i];
	}
	result.push_back(tmp);
return (result);
}

void HttpRequests::header_accept_validator(){
	if (requestHeaderMap.contains("accept")){
		bool valid_accept = false;
		std::vector<std::string> data;
		data = stov(requestHeaderMap["accept"]);
		std::vector<std::string> validAccepts = {"text/html", "application/xhtml+xml", "application/json", "image/webp", "*/*", "text/html, application/json;q=0.9"};
		for(std::string va:validAccepts)
		{
			if (valid_accept == true)
				valid_accept = false;
			for(std::string d:data){
				if(d == va)
					valid_accept = true;
			}
			if(valid_accept == false)
				throw WebServErr::BadRequestException("accept value is Not Acceptable");
			
		}
	}
	else{
		requestHeaderMap["accept"] = "*/*";
	}
}


void HttpRequests::header_contenttype_validator(){
	if (requestHeaderMap.contains("content-type")){
		bool valid_accept = false;
		std::vector<std::string> validAccepts = {"application/json", "application/x-www-form-urlencoded", "multipart/form-data", "text/plain", "application/xml"};
		for(std::string im:validAccepts)
		{
			if(requestLineMap["content-type"] == im)
				valid_accept = true;
		}
		if (!valid_accept)
			throw WebServErr::BadRequestException("content-type is Not Acceptable");
	}
	else{
		requestHeaderMap["content-type"] = "application/json";
	}
}

void HttpRequests::validateRequestHeader(void){
	host_validator();
	content_length_validator();
	header_connection_validator();
	header_accept_validator();
	// header_contenttype_validator();
}


// how many character till body part counter
void HttpRequests::tillBodyCounter(size_t &i, size_t requestLength, const std::vector<char> &request){

	for (; i <= requestLength; i++)
			{
				if(request[i] == '\r' && request[i+1]&&  request[i+1] == '\n' &&request[i+2] && request[i+2] == '\r' && request[i+3] && request[i+3] == '\n')
				{
					break;
				}
				upToBodyCounter++;
			}
}




HttpRequests &HttpRequests::httpParser(const std::vector<char> &request){

	size_t requestLength = request.size();
	size_t i = 0;
	
	// will extract the first line as a request line.
	if(!extractRequestLine(i, requestLength, request))
		std::cerr<<"extractRequestLine";
	validateRequestLine();

	// will extract the request header part as a request line.
	if(!extractRequestHeader(i, requestLength, request))
			std::cerr<<"extractRequestHeader";
	validateRequestHeader();
	for(const auto & pair:requestHeaderMap)
		std::cout<<pair.first<<": "<<pair.second<<std::endl;

	i = 0;
	tillBodyCounter(i, requestLength, request); 
	std::cout<<"up to body counter: "<<upToBodyCounter<<std::endl;
return(*this);

}


size_t HttpRequests::getupToBodyCounter(){
	return upToBodyCounter;
}

std::string HttpRequests::getrequestServerName(){
	return requestServerName;
}

unsigned int HttpRequests::getrequestPort(){
	return requestPort;
}

std::unordered_map<std::string, std::string> HttpRequests::getrequestLineMap(){
	return requestLineMap;
}
std::unordered_map<std::string, std::string> HttpRequests::getrequestHeaderMap(){
	return requestHeaderMap;
}
