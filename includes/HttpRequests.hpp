#pragma once
#include <unordered_map>
#include <vector>
#include <iostream>
#include "../includes/WebServErr.hpp"

class HttpRequests{
	private:
		size_t upToBodyCounter;
		std::string requestString;
		std::string requestLine;
		std::string requestHeader;
		std::string requestBody;
		std::string requestServerName;
		unsigned int requestPort;
		std::unordered_map<std::string, std::string> requestLineMap;
		std::unordered_map<std::string, std::string> requestHeaderMap;


	public:
		HttpRequests();
		HttpRequests(const HttpRequests &obj);
		HttpRequests operator=(const HttpRequests &obj);
		~HttpRequests();

		HttpRequests &httpParser(const std::vector<char> &request);

		void tillBodyCounter(size_t &i, size_t requestLength, const std::vector<char> &request);
		bool extractRequestLine(size_t &i, size_t requestLength, const std::vector<char> &request);
		bool extractRequestHeader(size_t &i, size_t requestLength, const std::vector<char> &request);

		void validateRequestLine();
		void validateMethod();
		void validateTarget();
		void validateHttpVersion();

		void validateRequestHeader(void);
		void host_validator(void);
		void content_length_validator(void);
		void header_connection_validator(void);
		void header_accept_validator();
		//getters
		size_t getupToBodyCounter();
		std::string getrequestServerName();
		unsigned int getrequestPort();
		std::unordered_map<std::string, std::string> getrequestLineMap();
		std::unordered_map<std::string, std::string> getrequestHeaderMap();

	};