#pragma once

#include <ctime>
#include <iostream>

typedef enum	e_priority
{
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL,
}	t_priority;

class LogSys{
private:
	static t_priority	priority_;
	LogSys();
	LogSys(const LogSys &copy) = delete;
	LogSys &operator=(const LogSys &copy) = delete;

	template<typename... Args>
	static void	log(const char* msg_priority_str, t_priority message_priority, const char* message, Args... args);

public:
	~LogSys();

	static void	setPriority(t_priority alert);
	static LogSys&	getLogSys();
	
	//Log Functions
	template<typename... Args>
	static void	trace(const char* message, Args... args);
	template<typename... Args>
	static void	debug(const char* message, Args... args);
	template<typename... Args>
	static void	info(const char* message, Args... args);
	template<typename... Args>
	static void	warn(const char* message, Args... args);
	template<typename... Args>
	static void	error(const char* message, Args... args);
	template<typename... Args>
	static void	fatal(const char* message, Args... args);
};


#define LOG_TRACE(Message, ...) (LogSys::trace(__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_DEBUG(Message, ...) (LogSys::debug(__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_INFO(Message, ...) (LogSys::info(__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_WARN(Message, ...) (LogSys::warn(__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_ERROR(Message, ...) (LogSys::error(__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_CRITICAL(Message, ...) (LogSys::fatal(__LINE__, __FILE__, Message, __VA_ARGS__))
