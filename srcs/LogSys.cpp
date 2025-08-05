#include "../includes/LogSys.hpp"

LogSys::LogSys()
{
	priority_ = TRACE;
}

LogSys::~LogSys()
{}

LogSys	&LogSys::getLogSys()
{
	static LogSys logger;
	return (logger);
}

void	LogSys::setPriority(t_priority alert)
{
	getLogSys().priority_ = alert;
}

template<typename... Args>
void	LogSys::log(const char* msg_priority_str, t_priority message_priority, const char* message, Args... args)
{
	if (priority_ <= message_priority)
	{
		std::time_t	current_time = std::time(0);
		std::tm*	timestamp = std::localtime(&current_time);
		std::cout << timestamp << " ";
		std::cout << msg_priority_str << " " << message << "\033[0m " << args... << std::endl;
	}
}

template<typename... Args>
void	LogSys::trace(const char *message, Args args...)
{
	getLogSys().log("\033[35m[Trace]\t",  message, args...);
}

template<typename... Args>
void	LogSys::debug(const char *message, Args args...)
{
	getLogSys().log("\033[34m[Debug]\t", message, args...);
}

template<typename... Args>
void	LogSys::info(const char *message, Args args...)
{
	getLogSys().log("\033[32m[Info]\t", message, args...);
}

template<typename... Args>
void	LogSys::warn(const char *message, Args args...)
{
	getLogSys().log("\033[33[Warning]\t", message, args...);
}

template<typename... Args>
void	LogSys::error(const char *message, Args args...)
{
	getLogSys().log("\033[38m[Error]\t", message, args...);
}

template<typename... Args>
void	LogSys::fatal(const char *message, Args args...)
{
	getLogSys().log("\033[31m[Fatal]\t", message, args...);
}

