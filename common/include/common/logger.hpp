#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include "sip_message.hpp"

namespace common {

    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    class Logger
    {
    public:

        static Logger& instance(const std::string& filename = "");

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void log(const std::string& component,
                 const std::string& call_id,
                 const std::string& state,
                 const std::string& message);

        void log(LogLevel level,
         const std::string& component,
         const std::string& call_id,
         const std::string& state,
         const std::string& message);

        void log(LogLevel level,
         const std::string& component,
         const std::string& call_id,
         const std::string& state,
         const common::SIPMessage& sip_message);

        void log(LogLevel level,
         const std::string& component,
         const std::string& call_id,
         const common::SIPMessage& sip_message);
        void separator();

    private:

        explicit Logger(const std::string& filename);



        static std::string timestamp();
        static std::string thread_id();

        std::mutex m_mutex;
        std::ofstream m_file;
    };
}