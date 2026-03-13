#include "common/logger.hpp"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <filesystem>

namespace common {

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