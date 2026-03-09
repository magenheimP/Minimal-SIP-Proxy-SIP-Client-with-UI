#pragma once

#include <string>
#include <mutex>

namespace common {
    class Logger
    {
    public:

        static Logger& instance();

        void log(const std::string& component,
                 const std::string& call_id,
                 const std::string& state,
                 const std::string& message);

    private:

        Logger() = default;

        static std::string timestamp();
        static std::string thread_id();

        std::mutex m_mutex;
    };
}