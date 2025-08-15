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
		std::unordered_map<std::string, std::string> requestMap;


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
		void header_contenttype_validator();
		void pre_validator(size_t requestLength, const std::vector<char> &request);

		//getters
		size_t getupToBodyCounter();
		std::unordered_map<std::string, std::string> getrequestMap();
		std::vector<std::string> stov(std::string &string, char c);
	};