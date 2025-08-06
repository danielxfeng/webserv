#pragma once

#include <ctime>
#include <iostream>
#include <iomanip>
#include <utility>

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
	t_priority	priority_ = TRACE;

	LogSys() {};

	LogSys(const LogSys &copy) = delete;
	LogSys &operator=(const LogSys &copy) = delete;

	static LogSys&	getLogSys()
	{
		static LogSys logger;
		return (logger);
	}

	template<typename... Args>
	void	log(const char* msg_priority_str, t_priority message_priority, const char* message, Args&&... args)
	{
		if (priority_ <= message_priority)
		{
			std::time_t	current_time = std::time(nullptr);
			std::tm*	timestamp = std::localtime(&current_time);
			std::cout << std::put_time(timestamp, "%H:%M:%S");
			std::cout << " " << msg_priority_str << " " << message << "\033[0m ";
			(std::cout << ... << std::forward<Args>(args));
			std::cout << std::endl;
		}
	}

public:
	~LogSys() {};

	static t_priority	getPriority()
	{
		return (getLogSys().priority_);
	}

	static void	setPriority(t_priority alert)
	{
		getLogSys().priority_ = alert;
	}


	//Log Functions
	template<typename... Args>
	static void	trace(const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;35m[Trace]\t", TRACE, message, args...);
	}

	template<typename... Args>
	static void	debug(const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;34m[Debug]\t", DEBUG, message, args...);
	}

	template<typename... Args>
	static void	info(const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;32m[Info]\t\t", INFO, message, args...);
	}

	template<typename... Args>
	static void	warn(const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;33m[Warning]\t", WARN, message, args...);
	}

	template<typename... Args>
	static void	error(const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;36m[Error]\t", ERROR, message, args...);
	}

	template<typename... Args>
	static void	fatal(const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;31m[Fatal]\t", FATAL, message, args...);
	}
};


#define LOG_TRACE(Message, ...) (LogSys::trace(Message, __VA_ARGS__))
#define LOG_DEBUG(Message, ...) (LogSys::debug(Message, __VA_ARGS__))
#define LOG_INFO(Message, ...) (LogSys::info(Message, __VA_ARGS__))
#define LOG_WARN(Message, ...) (LogSys::warn(Message, __VA_ARGS__))
#define LOG_ERROR(Message, ...) (LogSys::error(Message, __VA_ARGS__))
#define LOG_FATAL(Message, ...) (LogSys::fatal(Message, __VA_ARGS__))
