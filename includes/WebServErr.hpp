#pragma once

#include <string>
#include <exception>
#include <format>

class WebServErr
{
    WebServErr() = default;
    WebServErr(const WebServErr &o) = default;
    WebServErr &operator=(const WebServErr &o) = default;
    ~WebServErr() = default;

public:
    class SysCallErrException : public std::exception
    {
    private:
        std::string what_;

    public:
        SysCallErrException() = delete;
        explicit SysCallErrException(const std::string &what_arg);
        SysCallErrException(const SysCallErrException &other) = default;
        SysCallErrException &operator=(const SysCallErrException &o) = delete;
        ~SysCallErrException() override = default;

        const char *what() const noexcept override;
    };
};
