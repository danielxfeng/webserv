#pragma once

#include <ctime>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <type_traits>
#include <concepts>

typedef enum	e_priority
{
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL,
}	t_priority;

// Checks if Args are types that can be streamed to std::cout
template<typename T>
concept Streamable = requires(std::ostream& os, T t) {
    { os << t } -> std::same_as<std::ostream&>;
};

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

	template<Streamable... Args>
	void	log(const char* msg_priority_str, t_priority message_priority,
	  const char* function, const char* message, Args&&... args)
	{
		if (priority_ <= message_priority)
		{
			auto now = std::chrono::system_clock::now();
			auto now_time_t = std::chrono::system_clock::to_time_t(now);
			auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
			std::tm local_tm = *std::localtime(&now_time_t);
			std::cout << std::put_time(&local_tm, "%H:%M:%S");
			std::cout << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
			std::cout << " " << msg_priority_str << " " << message << " " << function << "\033[0m ";
			(std::cout << ... << std::forward<Args>(args));
			std::cout <<std::endl;
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
	static void	trace(const char* function, const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;35m[Trace]\t", TRACE, function, message, args...);
	}

	template<typename... Args>
	static void	debug(const char* function, const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;34m[Debug]\t", DEBUG, function, message, args...);
	}

	template<typename... Args>
	static void	info(const char* function, const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;32m[Info]\t", INFO, function, message, args...);
	}

	template<typename... Args>
	static void	warn(const char* function, const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;33m[Warning]\t", WARN, function, message, args...);
	}

	template<typename... Args>
	static void	error(const char* function, const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;36m[Error]\t", ERROR, function, message, args...);
	}

	template<typename... Args>
	static void	fatal(const char* function, const char* message, Args&&... args)
	{
		getLogSys().log("\033[0;31m[Fatal]\t", FATAL, function, message, args...);
	}
};


#define LOG_TRACE(Message, ...) (LogSys::trace(__FUNCTION__, Message, __VA_ARGS__))
#define LOG_DEBUG(Message, ...) (LogSys::debug(__FUNCTION__, Message, __VA_ARGS__))
#define LOG_INFO(Message, ...) (LogSys::info(__FUNCTION__, Message, __VA_ARGS__))
#define LOG_WARN(Message, ...) (LogSys::warn(__FUNCTION__, Message, __VA_ARGS__))
#define LOG_ERROR(Message, ...) (LogSys::error(__FUNCTION__, Message, __VA_ARGS__))
#define LOG_FATAL(Message, ...) (LogSys::fatal(__FUNCTION__, Message, __VA_ARGS__))
