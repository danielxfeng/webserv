#include <gtest/gtest.h>
#include "../../includes/HttpRequests.hpp"
#include "../../includes/WebServErr.hpp"




TEST(TargetTestDecoding, invaidvalue) {
	std::string request =
        "GET /index%.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

HttpRequests parser;
EXPECT_THROW(parser.httpParser(request), WebServErr::BadRequestException);

}

TEST(TargetTestDecoding, invaidvalue2) {
	std::string request =
        "GET /index%%.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

HttpRequests parser;
EXPECT_THROW(parser.httpParser(request), WebServErr::BadRequestException);
}
