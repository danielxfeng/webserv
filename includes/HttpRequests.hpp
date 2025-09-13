#pragma once
#include <unordered_map>
#include <vector>
#include <iostream>
#include <algorithm>
#include "SharedTypes.hpp"
#include "WebServErr.hpp"
#include "LogSys.hpp"

class HttpRequests
{
private:
	size_t upToBodyCounter;
	std::unordered_map<std::string, std::string> requestHeaderMap;
	std::unordered_map<std::string, std::string> requestLineMap;
	std::unordered_map<std::string, std::string> requestBodyMap;
	bool is_chunked;

public:
	HttpRequests();
	HttpRequests(const HttpRequests &obj);
	HttpRequests operator=(const HttpRequests &obj);
	~HttpRequests();

	void httpParser(const std::string_view &request);

	void tillBodyCounter(size_t requestLength, const std::string_view &request);

	void extractRequestLine(size_t &i, size_t requestLength, const std::string_view &request);
	void extractRequestHeader(size_t &i, size_t requestLength, const std::string_view &request);
	void extractRequestBody(size_t &i, size_t requestLength, const std::string_view &request);
	void validateRequestLine();
	void validateMethod();
	void validateTarget();
	std::string httpTargetDecoder(std::string &target);
	void validateHttpVersion();

	void validateRequestHeader(void);
	void host_validator(void);
	void content_length_validator(void);
	void header_connection_validator(void);
	void header_contenttype_validator();
	void header_transfer_encoding_validator();
	void pre_validator(size_t requestLength, const std::string_view &request);

	void parse_body_header(std::string_view requestBodyHeader);
	void validateRequestBody(void);
	void validateFileName(void);
	void validateContentType(void);

	// getters
	size_t getupToBodyCounter();

	std::unordered_map<std::string, std::string> getrequestHeaderMap();
	std::unordered_map<std::string, std::string> getrequestLineMap();
	std::unordered_map<std::string, std::string> getrequestBodyMap();

	std::string getHttpVersion();
	std::string getHttpRequestMethod();
	std::vector<std::string> stov(std::string &string, char c);
	bool is_digit_str(std::string &str);
	bool isChunked();
};