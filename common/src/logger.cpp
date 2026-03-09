#include "common/logger.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::log(const std::string& component,
                 const std::string& call_id,
                 const std::string& state,
                 const std::string& message)
{
    std::lock_guard lock(m_mutex);

    std::cout
        << "[" << timestamp() << "] "
        << "[" << thread_id() << "] "
        << "[" << component << "] "
        << "[CallID=" << call_id << "] "
        << "[" << state << "] "
        << message
        << std::endl;
}

std::string Logger::timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&time, &tm);

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}

std::string Logger::thread_id()
{
    std::ostringstream ss;
    ss << "Thread-" << std::this_thread::get_id();
    return ss.str();
}