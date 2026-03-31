#include "common/logger.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <filesystem>

namespace common {

    std::string to_string(LogLevel level)
    {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }

    Logger& Logger::instance(const std::string& filename)
    {
        static Logger logger(filename.empty() ? "default.log" : filename);
        return logger;
    }
    Logger::Logger(const std::string& filename)
    {
        std::filesystem::create_directories("logs");

        m_file.open("logs/" + filename, std::ios::app);
    }


    void Logger::log(const std::string& component,
                     const std::string& call_id,
                     const std::string& state,
                     const std::string& message)
    {
        std::lock_guard lock(m_mutex);

        std::ostringstream log_line;

        log_line
            << "[" << timestamp() << "] "
            << "[" << thread_id() << "] "
            << "[" << component << "] "
            << "[CallID=" << call_id << "] "
            << "[" << state << "] "
            << message;

        std::string output = log_line.str();

        //std::cout << output << std::endl;

        if (m_file.is_open()) {
            m_file << output << std::endl;
            m_file.flush();
        }
    }

    void Logger::log(LogLevel level, const std::string &component, const std::string &call_id, const std::string &state,
        const std::string &message) {

        std::lock_guard<std::mutex> lock(m_mutex);

        std::ostringstream log_line;

        log_line << "[" << to_string(level) << "] "
               << "[" << component << "] "
               << "[" << call_id << "] "
               << "[" << state << "] "
               << message;

        std::string output = log_line.str();
        //std::cout << output << std::endl;

        if (m_file.is_open()) {
            m_file << output << std::endl;
            m_file.flush();
        }

    }

    void Logger::log(LogLevel level, const std::string &component, const std::string &call_id, const std::string &state,
        const common::SIPMessage &sip_message) {

        std::lock_guard<std::mutex> lock(m_mutex);

        std::ostringstream log_line;

        log_line << "[" << to_string(level) << "] "
       << "[" << component << "] "
       << "[" << call_id << "] "
       << "[" << state << "] "
       << sip_message.serialize();

        std::string output = log_line.str();
        //std::cout << output << std::endl;

        if (m_file.is_open()) {
            m_file << output << std::endl;
            m_file.flush();
        }


    }

    void Logger::log(LogLevel level, const std::string &component, const std::string &call_id,
        const common::SIPMessage &sip_message) {

        std::lock_guard<std::mutex> lock(m_mutex);

        std::ostringstream log_line;

        log_line << "[" << to_string(level) << "] "
       << "[" << component << "] "
       << "[" << call_id << "] "
       << sip_message.serialize();

        std::string output = log_line.str();
        //std::cout << output << std::endl;

        if (m_file.is_open()) {
            m_file << output << std::endl;
            m_file.flush();
        }
    }

    void Logger::separator()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file << std::endl;
            m_file.flush();
        }
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
}