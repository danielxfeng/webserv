#pragma once

#include <string>
#include <exception>
#include <sstream>

typedef enum e_error_codes
{
	ERR_301,
	ERR_401,
	ERR_403,
	ERR_404,
	ERR_409,
	ERR_500
}	t_error_codes;

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

	class MethodException: public std::exception
	{
		private:
			std::string what_;
		public:
			MethodException() = delete;
			explicit MethodException(t_error_codes code, const std::string &what_arg);
			MethodException(const MethodException &other) = default;
			MethodException &operator=(const MethodException &other) = delete;
			~MethodException() override = default;
	};

	class UtilsException: public std::exception
	{
		private:
			std::string what_;
		public:
			UtilsException() = delete;
			explicit UtilsException(const std::string &what_arg);
			UtilsException(const UtilsException &other) = default;
			UtilsException &operator=(const UtilsException &other) = delete;
			~UtilsException() override = default;
	};
};
