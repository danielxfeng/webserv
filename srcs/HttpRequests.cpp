#include "../includes/HttpRequests.hpp"

// default
HttpRequests::HttpRequests() : upToBodyCounter(0), requestHeaderMap(),
							   requestLineMap(), requestBodyMap()
{
}
/*
std::string_view sv = string;
sv.substr()
*/

HttpRequests::HttpRequests(const HttpRequests &obj)
{
	if (this != &obj)
	{
		upToBodyCounter = obj.upToBodyCounter;
		requestHeaderMap = obj.requestHeaderMap;
		requestLineMap = obj.requestLineMap;
		requestBodyMap = obj.requestBodyMap;
	}
};
HttpRequests HttpRequests::operator=(const HttpRequests &obj)
{
	if (this != &obj)
	{
		upToBodyCounter = obj.upToBodyCounter;
		requestHeaderMap = obj.requestHeaderMap;
		requestLineMap = obj.requestLineMap;
		requestBodyMap = obj.requestBodyMap;
	}
	return (*this);
};
HttpRequests::~HttpRequests() {
};
/**
 * @brief extract the requestline from the request,
	then stores them into the map.
 * @param (size_t &i, size_t requestLength, const std::string &request)
 * @return nothing
 */
bool HttpRequests::extractRequestLine(size_t &i, size_t requestLength,
									  const std::string &request)
{
	std::string method;
	std::string target;
	std::string httpVersion;
	std::string requestLine;
	size_t j = 0;
	for (; i <= requestLength; i++)
	{
		if (request[i] == '\r' && request[i + 1] == '\n')
		{
			i = i + 2;
			break;
		}
		requestLine += request[i];
	}
	j = 0;
	for (; j < requestLine.size(); j++)
	{
		if (requestLine[j] == ' ')
		{
			j += 1;
			break;
		}
		method += requestLine[j];
	}
	for (; j < requestLine.size(); j++)
	{
		if (requestLine[j] == ' ')
		{
			j += 1;
			break;
		}
		target += requestLine[j];
	}
	for (; j < requestLine.size(); j++)
	{
		httpVersion += requestLine[j];
	}
	requestLineMap["Method"] = method;
	requestLineMap["Target"] = target;
	requestLineMap["HttpVersion"] = httpVersion;
	return (true);
}

/**
 * @brief validate the HTTP version and we accept 1.1 and 1.0
 * @param nothing
 * @return nothing
 */
void HttpRequests::validateHttpVersion()
{
	if (requestLineMap["HttpVersion"].empty())
		throw WebServErr::BadRequestException("Http version must be 1.1 ot 1.0");
	if (!(requestLineMap["HttpVersion"] == "HTTP/1.1" || requestLineMap["HttpVersion"] == "HTTP/1.0"))
		throw WebServErr::BadRequestException("Http version must be 1.1 ot 1.0");
}

void HttpRequests::validateTarget()
{
	if (requestLineMap["Target"].empty())
		throw WebServErr::BadRequestException("target cannot be empty");
	std::string invalidCharactersUri = " <>\"{}|\\^`";
	for (char j : requestLineMap["Target"])
	{
		for (char i : invalidCharactersUri)
		{
			if (i == j)
				throw WebServErr::BadRequestException("target cannot has invalid characters");
		}
	}
}
/**
 * @brief validate the method it must be get, post and delete.
 * @param (size_t &i, size_t requestLength, const std::string &request)
 * @return nothing
 */
void HttpRequests::validateMethod()
{
	bool valid_method;

	valid_method = false;
	std::vector<std::string> validMethods = {"GET", "POST", "DELETE"};
	for (std::string im : validMethods)
	{
		if (requestLineMap["Method"] == im)
			valid_method = true;
	}
	if (!valid_method)
		throw WebServErr::BadRequestException("Invalid method only GET,	POST and DELETE are allowed");
}

/**
 * @brief vaalidate the request line  from the request.
 * @param none
 * @return nothing
 */
void HttpRequests::validateRequestLine()
{
	// validate the method type
	validateMethod();
	validateTarget();
	validateHttpVersion();
}

// request header functions

/**
 * @brief extract the request body from the request.
 * @param (size_t &i, size_t requestLength, const std::string &request)
 * @return nothing
 */
bool HttpRequests::extractRequestHeader(size_t &i, size_t requestLength,
										const std::string &request)
{
	bool secondPartBool;
	size_t j;

	std::string firstPart;
	std::string secondPart;
	std::string requestHeader;
	secondPartBool = false;
	for (; i <= requestLength; i++)
	{
		if (request[i] == '\r' && request[i + 1] && request[i + 1] == '\n' && request[i + 2] && request[i + 2] == '\r' && request[i + 3] && request[i + 3] == '\n')
		{
			i += 4;
			break;
		}
		requestHeader += request[i];
	}
	requestHeader.append("\r\n");
	j = 0;
	for (; j < requestHeader.length(); j++)
	{
		// TODO : u can use std::find and others and read bout it.
		if (requestHeader[j] == '\r' && requestHeader[j + 1] && requestHeader[j + 1] == '\n')
		{
			j += 2;
			secondPartBool = false;
			// to make sure that Host and content-length has only one value,it is not allowed to be duplicated.
			if ((requestHeaderMap.contains("host") && firstPart == "host") || (requestHeaderMap.contains("content-length") && firstPart == "content-length"))
			{
				std::cerr << "Error: we have host before" << std::endl;
				break;
			}
			requestHeaderMap[firstPart] = secondPart;
			firstPart = "";
			secondPart = "";
		}
		if (requestHeader[j] == ':' && requestHeader[j + 1] && requestHeader[j + 1] == ' ')
		{
			secondPartBool = true;
			j += 2;
		}
		if (!secondPartBool)
			firstPart += std::tolower(requestHeader[j]);
		if (secondPartBool)
			secondPart += std::tolower(requestHeader[j]);
	}
	return (true);
}

/**
 * @brief validate the host and split it to 2 parts servername and port
 * @param nothing
 * @return nothing
 */
void HttpRequests::host_validator(void)
{
	bool secondPartBool;

	std::string host_str;
	std::string firstPart;
	std::string secondPart;
	secondPartBool = false;
	if (!requestHeaderMap.contains("host"))
		throw WebServErr::BadRequestException("host is required");
	host_str = requestHeaderMap["host"];
	for (size_t i = 0; i < host_str.length(); i++)
	{
		if (host_str[i] == ':')
		{
			secondPartBool = true;
			i++;
		}
		if (!secondPartBool)
			firstPart += host_str[i];
		else
			secondPart += host_str[i];
	}
	requestHeaderMap["servername"] = firstPart;
	// TODO check if the ServerName is valid from the list.
	// validate Port
	if (secondPartBool)
	{
		if ((std::stoi(secondPart) < 1 || std::stoi(secondPart) > 65535))
			throw WebServErr::BadRequestException("post is out of allowed range from 1 to 655535");
		requestHeaderMap["requestport"] = secondPart;
	}
}

/**
 * @brief validate the content-length of the body
 * @param nothing
 * @return nothing
 */
void HttpRequests::content_length_validator(void)
{
	size_t content_length_var;

	content_length_var = 0;
	if (requestLineMap["Method"] == "POST" || requestLineMap["Method"] == "DELETE")
	{
		if (!requestHeaderMap.contains("content-length") && !requestHeaderMap.contains("transfer-encoding"))
			throw WebServErr::BadRequestException("POST must have content-length or transfer-encoding");
		if (requestHeaderMap.contains("content-length"))
		{
			if (requestHeaderMap["content-length"].empty())
				throw WebServErr::BadRequestException("POST and DELETE must have content-length value");
			else
			{
				content_length_var = static_cast<size_t>(stoull(requestHeaderMap["content-length"]));
				if (content_length_var > 1073741824)
					throw WebServErr::BadRequestException("Bad content-length less than 1GB are allowed");
			}
		}
	}
	else if (requestLineMap["Method"] == "GET")
		if (requestHeaderMap.contains("content-length"))
			throw WebServErr::BadRequestException("GET must have no content-length");
}

/**
 * @brief vaalidate the connection value of the boody of the request
 * @param nothing
 * @return nothing
 */
void HttpRequests::header_connection_validator(void)
{
	if (requestHeaderMap.contains("connection"))
	{
		if (!(requestHeaderMap["connection"] == "keep-alive" || requestHeaderMap["connection"] == "close"))
			throw WebServErr::BadRequestException("Incrorrect connection value,	must be keep-alive or close");
	}
	else
	{
		requestHeaderMap["connection"] = "keep-alive";
	}
}

/**
 * @brief channge the string to vector of strings
 * @param (std::string &string, char c) it takes a string and c delimiter.
 * @return nothing
 */
std::vector<std::string> HttpRequests::stov(std::string &string, char c)
{
	std::vector<std::string> result;
	std::string tmp;
	for (size_t i = 0; i < string.length(); i++)
	{
		if (string[i] == c)
		{
			result.push_back(tmp);
			tmp = "";
			if (string[i] && string[i + 1] == ' ')
				i++;
			i++;
		}
		tmp += string[i];
	}
	result.push_back(tmp);
	return (result);
}

/**
 * @brief validate the content-length part in header
 * @param nothing
 * @return nothing
 */
void HttpRequests::header_contenttype_validator()
{
	bool has_semicolon;
	bool valid_types;

	if (requestLineMap["Method"] == "POST")
	{
		has_semicolon = false;
		std::string type = requestHeaderMap["content-type"];
		for (size_t i = 0; i < type.length(); i++)
		{
			if (type[i] == ';')
			{
				has_semicolon = true;
			}
		}
		if (has_semicolon)
		{
			std::vector<std::string> type = stov(requestHeaderMap["content-type"],';');
			requestHeaderMap["content-type"] = type[0];
			requestHeaderMap["boundary"] = type[1].substr(9);
		}
		valid_types = false;
		if (!requestHeaderMap.contains("content-type") || requestHeaderMap["content-type"].empty())
			throw WebServErr::BadRequestException("POST must have content-type value");
		std::vector<std::string> validAccepts = {"image/png",
												 "multipart/form-data"};
		for (std::string im : validAccepts)
		{
			if (requestHeaderMap["content-type"] == im)
				valid_types = true;
		}
		if (!valid_types)
			throw WebServErr::BadRequestException("content-type is Not Acceptable or not suppoted value");
	}
}

/**
 * @brief validate the request line part
 * @param nothing
 * @return nothing
 */
void HttpRequests::validateRequestHeader(void)
{
	host_validator();
	content_length_validator();
	header_connection_validator();
	header_contenttype_validator();
}

/**
 * @brief count the characters till the body part.
 * @param (size_t &i, size_t requestLength, const std::string &request)
 * @return nothing it store in the variable.
 */
void HttpRequests::tillBodyCounter(size_t &i, size_t requestLength,
								   const std::string &request)
{
	for (; i <= requestLength; i++)
	{
		if (request[i] == '\r' && request[i + 1] && request[i + 1] == '\n' && request[i + 2] && request[i + 2] == '\r' && request[i + 3] && request[i + 3] == '\n')
		{
			break;
		}
		if (upToBodyCounter > 8192)
		{ // 8KB max size of the requestline + request header
			throw WebServErr::BadRequestException("403 Long request");
		}
		upToBodyCounter++;
	}
}
/**
 * @brief validate the reauest before start extraction.
 * @param (size_t requestLength, const std::string &request)
 * @return nothing it store in the variable.
 */
void HttpRequests::pre_validator(size_t requestLength,
								 const std::string &request)
{
	(void)requestLength;
	for (size_t i = 0; i <= upToBodyCounter; i++)
	{
		if (request[i] == ',' && request[i + 1] && request[i + 1] == ',')
			throw WebServErr::BadRequestException("Invalid Http request,it has extra (,).");
	}
}

void HttpRequests::parse_body_header(std::string_view requestBodyHeader)
{
	bool colonFind;
	bool endofLine;

	std::string firstPart;
	std::string secondPart;

	for (size_t i=0; i < requestBodyHeader.size(); i++)
	{
		colonFind = false;
		endofLine = false;
		if (requestBodyHeader[i] == '\r' && requestBodyHeader[i + 1] == '\n' && requestBodyHeader[i + 2] == '\r' && requestBodyHeader[i + 3] == '\n')
		{
			break;
		}
		if (requestBodyHeader[i] == '\r' && requestBodyHeader[i] == '\n')
		{
			endofLine = true;
			requestBodyMap[firstPart] = secondPart;
			firstPart = "";
			secondPart = "";
		}
		if (requestBodyHeader[i] == ':')
		{
			i++;
			colonFind = true;
		}
		if (!colonFind)
			firstPart += std::tolower(requestBodyHeader[i]);
		if (colonFind && !endofLine)
			secondPart += std::tolower(requestBodyHeader[i]);
	}
}

bool HttpRequests::extractRequestBody(size_t &i, size_t requestLength,
									  const std::string &request)
{
	size_t pos;
	size_t boundarySize;

	pos = 0;
	(void)requestLength;
	std::string_view sv(request);
	std::string_view requestBody;
	std::string_view requestBodyHeader;
	requestBody = sv.substr(i, request.size());
	boundarySize = requestHeaderMap.contains("boundary") ? requestHeaderMap["boundary"].size() : 0;
	if (boundarySize == 0 || boundarySize > requestBody.size())
		throw WebServErr::BadRequestException("must have boundary");
	// extract the first boundary line
	requestBodyHeader = requestBody.substr(boundarySize + 4, requestLength);
	// find end of the header part of the body
	pos = requestBodyHeader.find("\r\n\r\n");
	if (pos == std::string::npos)
		throw WebServErr::BadRequestException("no file part found.");
	// extract the body header part
	requestBodyHeader = requestBodyHeader.substr(0, pos);
	parse_body_header(requestBodyHeader);
	std::cout << "requestBodyHeader :" << requestBodyHeader << std::endl;
	return (true);
}

/**
 * @brief Parsing the request.
 * @param std::string.
 * @return (HttpRequests&);
 */
HttpRequests &HttpRequests::httpParser(const std::string &request)
{
	size_t i;
	size_t requestLength;

	i = 0;
	requestLength = request.size();
	tillBodyCounter(i, requestLength, request);
	i = 0;
	// will extract the first line as a request line.
	pre_validator(requestLength, request);
	if (!extractRequestLine(i, requestLength, request))
		std::cerr << "extractRequestLine";
	validateRequestLine();
	// will extract the request header part as a request line.
	if (!extractRequestHeader(i, requestLength, request))
		std::cerr << "extractRequestHeader";
	validateRequestHeader();
	if (!extractRequestBody(i, requestLength, request))
		std::cerr << "extractRequestHeader";
	// validateRequestBody();
	// for (const auto &pair : requestLineMap)
	// 	std::cout << pair.first << ": " << pair.second << std::endl;
	// for (const auto &pair : requestHeaderMap)
	// 	std::cout << pair.first << ": " << pair.second << std::endl;
	for (const auto &pair : requestBodyMap)
		std::cout << pair.first << ": " << pair.second << std::endl;
	return (*this);
}

/**
 * @brief Return the number of characters till the body part of the request.
 * @param void.
 * @return size_t.
 */
size_t HttpRequests::getupToBodyCounter()
{
	return (upToBodyCounter);
}

/**
 * @brief Return the parsed map.
 * @param void.
 * @return std::unordered_map<std::string, std::string>.
 */
std::unordered_map<std::string, std::string> HttpRequests::getrequestHeaderMap()
{
	return (requestHeaderMap);
}

std::unordered_map<std::string, std::string> HttpRequests::getrequestLineMap()
{
	return (requestLineMap);
}

std::unordered_map<std::string, std::string> HttpRequests::getrequestBodyMap()
{
	return (requestBodyMap);
}

std::string HttpRequests::getHttpVersion()
{
	return (requestLineMap["HttpVersion"]);
}