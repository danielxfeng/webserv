#pragma once

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <queue>

typedef enum e_priority {
    TRACE, DEBUG, INFO, WARN, ERROR, FATAL
} t_priority;

struct LogMessage {
    std::string priority_str;
    t_priority priority;
    std::string function;
    std::string message;
};

class LogSys {
private:
    t_priority priority_ = TRACE;
    std::queue<LogMessage> logQueue;
    const size_t autoFlushSize = 10; // Flush automatically when queue reaches this size

    LogSys() {}
    LogSys(const LogSys&) = delete;
    LogSys& operator=(const LogSys&) = delete;

    static LogSys& getLogSys() {
        static LogSys logger;
        return logger;
    }

    template<typename... Args>
    std::string formatMessage(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        return oss.str();
    }

    template<typename... Args>
    void enqueue(const char* priority_str, t_priority prio,
                 const char* function, Args&&... args) 
    {
        if(priority_ <= prio) {
            logQueue.push({priority_str, prio, function, formatMessage(std::forward<Args>(args)...)});
            if(logQueue.size() >= autoFlushSize) {
                flush(); // Automatically flush when queue grows too large
            }
        }
    }

    void flush() {
        while(!logQueue.empty()) {
            const auto& msg = logQueue.front();

            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            std::tm local_tm = *std::localtime(&now_time_t);

            std::cout << std::put_time(&local_tm, "%H:%M:%S")
                      << '.' << std::setfill('0') << std::setw(3) << now_ms.count()
                      << " " << msg.priority_str
                      << " Func: " << msg.function
                      << ", Msg: " << msg.message
                      << "\033[0m" << std::endl;

            logQueue.pop();
        }
    }

public:
    static t_priority getPriority() { return getLogSys().priority_; }
    static void setPriority(t_priority p) { getLogSys().priority_ = p; }

    template<typename... Args>
    static void trace(const char* function, Args&&... args) {
        getLogSys().enqueue("\033[0;35m[Trace]\t", TRACE, function, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(const char* function, Args&&... args) {
        getLogSys().enqueue("\033[0;34m[Debug]\t", DEBUG, function, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(const char* function, Args&&... args) {
        getLogSys().enqueue("\033[0;32m[Info]\t", INFO, function, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(const char* function, Args&&... args) {
        getLogSys().enqueue("\033[0;33m[Warning]\t", WARN, function, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const char* function, Args&&... args) {
        getLogSys().enqueue("\033[0;36m[Error]\t", ERROR, function, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void fatal(const char* function, Args&&... args) {
        getLogSys().enqueue("\033[0;31m[Fatal]\t", FATAL, function, std::forward<Args>(args)...);
    }

    // Manual flush
    static void flushLogs() {
        getLogSys().flush();
    }
};

#define LOG_TRACE(Message, ...) (LogSys::trace(__FUNCTION__, Message, ##__VA_ARGS__))
#define LOG_DEBUG(Message, ...) (LogSys::debug(__FUNCTION__, Message, ##__VA_ARGS__))
#define LOG_INFO(Message, ...) (LogSys::info(__FUNCTION__, Message, ##__VA_ARGS__))
#define LOG_WARN(Message, ...) (LogSys::warn(__FUNCTION__, Message, ##__VA_ARGS__))
#define LOG_ERROR(Message, ...) (LogSys::error(__FUNCTION__, Message, ##__VA_ARGS__))
#define LOG_FATAL(Message, ...) (LogSys::fatal(__FUNCTION__, Message, ##__VA_ARGS__))