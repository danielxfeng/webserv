#include "WebServErr.hpp"

WebServErr::SysCallErrException::SysCallErrException(const std::string &what_arg)
    : what_(std::format("Syscall error: {}", what_arg)) {}

const char *WebServErr::SysCallErrException::what() const noexcept
{
    return what_.c_str();
}

WebServErr::ShouldNotBeHereException::ShouldNotBeHereException(const std::string &what_arg)
    : what_(std::format("Should not be here: {}", what_arg)) {}

const char *WebServErr::ShouldNotBeHereException::what() const noexcept
{
    return what_.c_str();
}

WebServErr::BadRequestException::BadRequestException(const std::string &what_arg)
    : what_(std::format("Bad request: {}", what_arg)) {}

const char *WebServErr::BadRequestException::what() const noexcept
{
    return what_.c_str();
}

WebServErr::MethodException::MethodException(const std::string &what_arg)
	: what_(std::format(what_arg) {}

const char *WebServErr::MethodException::what() const noexcept
{
	return what_.c_str();
}

WebServErr::UtilsException::UtilsException(const std::string &what_arg)
	: what_(std::format(what_arg) {}

const char *WebServErr::UtilsException::what() const noexcept
{
	return what_.c_str();
}
