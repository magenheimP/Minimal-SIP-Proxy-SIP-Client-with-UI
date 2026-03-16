#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

#include "call_session.hpp"

namespace proxy
{

    class CallRegistry
    {
    public:
        std::shared_ptr<CallSession> create_call(const std::string& call_id,
                                                 const std::string& caller,
                                                 const std::string& callee);

        std::shared_ptr<CallSession> find_call(const std::string& call_id);

        void remove_call(const std::string& call_id);

    private:
        std::unordered_map<std::string, std::shared_ptr<CallSession>> m_calls;

        std::mutex m_mutex;
    };

}