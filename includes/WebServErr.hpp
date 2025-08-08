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

    class ShouldNotBeHereException : public std::exception
    {
    private:
        std::string what_;

    public:
        ShouldNotBeHereException() = delete;
        explicit ShouldNotBeHereException(const std::string &what_arg);
        ShouldNotBeHereException(const ShouldNotBeHereException &other) = default;
        ShouldNotBeHereException &operator=(const ShouldNotBeHereException &o) = delete;
        ~ShouldNotBeHereException() override = default;

        const char *what() const noexcept override;
    };

    class BadRequestException : public std::exception
    {
    private:
        std::string what_;

    public:
        BadRequestException() = delete;
        explicit BadRequestException(const std::string &what_arg);
        BadRequestException(const BadRequestException &other) = default;
        BadRequestException &operator=(const BadRequestException &o) = delete;
        ~BadRequestException() override = default;

        const char *what() const noexcept override;
    };
};
