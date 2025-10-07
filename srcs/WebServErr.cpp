#include "../includes/WebServErr.hpp"

std::string myFormat(const std::string &prefix, const std::string &what_arg)
{
    std::ostringstream oss;
    oss << prefix << " " << what_arg;
    return oss.str();
}

WebServErr::SysCallErrException::SysCallErrException(const std::string &what_arg)
    : what_(myFormat("Syscall error:", what_arg)) {}

const char *WebServErr::SysCallErrException::what() const noexcept
{
    return what_.c_str();
}

WebServErr::ShouldNotBeHereException::ShouldNotBeHereException(const std::string &what_arg)
    : what_(myFormat("Should not be here:", what_arg)) {}

const char *WebServErr::ShouldNotBeHereException::what() const noexcept
{
    return what_.c_str();
}




WebServErr::BadRequestException::BadRequestException(const std::string &what_arg)
    : what_(myFormat("Bad request:", what_arg)) {}

const char *WebServErr::BadRequestException::what() const noexcept
{
    return what_.c_str();
}


WebServErr::MethodException::MethodException(t_status_error_codes code, const std::string &what_arg)
	: what_(myFormat("Bad request:", what_arg)), code_(code) {}

const char *WebServErr::MethodException::what() const noexcept
{
	return what_.c_str();
}

t_status_error_codes WebServErr::MethodException::code() const noexcept
{
	return (code_);
}

WebServErr::UtilsException::UtilsException(const std::string &what_arg)
	: what_(myFormat("Bad request:", what_arg)) {}

const char *WebServErr::UtilsException::what() const noexcept
{
	return what_.c_str();
}



WebServErr::InvalidRequestHeader::InvalidRequestHeader(const std::string &what_arg)
	: what_(myFormat("Invalid request header:", what_arg)) {}
	
const char *WebServErr::InvalidRequestHeader::what() const noexcept
{
    return what_.c_str();
}

WebServErr::ErrorResponseException::ErrorResponseException(const std::string &what_arg)
	: what_(myFormat("Error Response Failed:", what_arg)) {}

const char *WebServErr::ErrorResponseException::what() const noexcept
{
	return what_.c_str();
}


WebServErr::InvalidCgiHeader::InvalidCgiHeader(const std::string &what_arg)
    : what_(myFormat("Bad request:", what_arg)) {}

const char *WebServErr::InvalidCgiHeader::what() const noexcept
{
    return what_.c_str();
}


WebServErr::CgiHeaderNotFound::CgiHeaderNotFound(const std::string &what_arg)
    : what_(myFormat("Bad request:", what_arg)) {}

const char *WebServErr::CgiHeaderNotFound::what() const noexcept
{
    return what_.c_str();
}