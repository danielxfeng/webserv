#pragma once

#include <string>
#include <exception>
#include <sstream>

#include "SharedTypes.hpp"

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

	// class MethodException: public std::exception
	// {
	// 	private:
	// 		std::string what_;
	// 	public:
	// 		MethodException() = delete;
	// 		explicit MethodException(t_status_error_codes code, const std::string &what_arg);
	// 		MethodException(const MethodException &other) = default;
	// 		MethodException &operator=(const MethodException &other) = delete;
	// 		~MethodException() override = default;
	// };


    class MethodException : public std::exception
    {
    private:
        std::string what_;

    public:
        MethodException() = delete;
        explicit MethodException(t_status_error_codes code, const std::string &what_arg);
        MethodException(const MethodException &other) = default;
        MethodException &operator=(const MethodException &o) = delete;
        ~MethodException() override = default;

        const char *what() const noexcept override;
    };





    class InvalidRequestHeader : public std::exception
    {
    private:
        std::string what_;

    public:
        InvalidRequestHeader() = delete;
        explicit InvalidRequestHeader(const std::string &what_arg);
        InvalidRequestHeader(const InvalidRequestHeader &other) = default;
        InvalidRequestHeader &operator=(const InvalidRequestHeader &o) = delete;
        ~InvalidRequestHeader() override = default;

        const char *what() const noexcept override;
    };


    class UtilsException : public std::exception
    {
    private:
        std::string what_;

    public:
        UtilsException() = delete;
        explicit UtilsException(const std::string &what_arg);
        UtilsException(const UtilsException &other) = default;
        UtilsException &operator=(const UtilsException &o) = delete;
        ~UtilsException() override = default;

        const char *what() const noexcept override;
    };

	class CGIException: public std::exception
	{
		private:
			std::string what_;
		public:
			CGIException() = delete;
			explicit CGIException(const std::string &what_arg);
			CGIException(const CGIException &other) = default;
			CGIException &operator=(const CGIException &other) = delete;
			~CGIException() override = default;
			const char *what() const noexcept override;

	};

	class CGIException: public std::exception
	{
		private:
			std::string what_;
		public:
			CGIException() = delete;
			explicit CGIException(const std::string &what_arg);
			CGIException(const CGIException &other) = default;
			CGIException &operator=(const CGIException &other) = delete;
			~CGIException() override = default;

			const char *what() const noexcept override;
	};
};
