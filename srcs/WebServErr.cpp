#include "WebServErr.hpp"

WebServErr::SysCallErrException::SysCallErrException(const std::string &what_arg)
    : what_(std::format("Syscall error: {}", what_arg)) {}

const char *WebServErr::SysCallErrException::what() const noexcept {
    return what_.c_str();
}
