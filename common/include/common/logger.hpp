#pragma once

#include <string>
#include <mutex>
#include <fstream>

namespace common {
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

    private:

        explicit Logger(const std::string& filename);



        static std::string timestamp();
        static std::string thread_id();

        std::mutex m_mutex;
        std::ofstream m_file;
    };
}