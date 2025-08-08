#pragma once
#include <unordered_map>
#include <vector>
#include <iostream>

class HttpRequests{
	private:
		std::string requestString;
		std::string requestLine;
		std::string requestHeader;
		std::string requestBody;
		std::unordered_map<std::string, std::string> requestLineMap;
		std::unordered_map<std::string, std::string> requestHeaderMap;
		std::unordered_map<std::string, std::string> requestBodyMap;

	public:
		HttpRequests();
		// HttpRequests(const HttpRequests &obj);
		// HttpRequests operator=(const HttpRequests &obj);
		~HttpRequests();
		HttpRequests &httpParser(const std::vector<char> &request);
	//helper functions
		bool extractRequestLine(size_t &i, size_t requestLength, const std::vector<char> &request);
		bool extractRequestHeader(size_t &i, size_t requestLength, const std::vector<char> &request);
		bool extractRequestBody(size_t &i, size_t requestLength, const std::vector<char> &request);

		//validaton functions:
		bool validateRequestLine();
		bool validateMethod();
		bool validateTarget();
		bool validateHttpVersion();

		bool validateRequestHeader();
		bool validateRequestBody();
	};