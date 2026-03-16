#include "proxy/call_registry.hpp"
#include "common/logger.hpp"

namespace proxy
{

    std::shared_ptr<CallSession> CallRegistry::create_call(
            const std::string& call_id,
            const std::string& caller,
            const std::string& callee)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto session = std::make_shared<CallSession>(call_id, caller, callee);

        m_calls[call_id] = session;

        common::Logger::instance().log(
            "Proxy",
            call_id,
            "INIT",
            "Call session created"
        );

        return session;
    }

    std::shared_ptr<CallSession> CallRegistry::find_call(const std::string& call_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_calls.find(call_id);

        if (it != m_calls.end())
            return it->second;

        return nullptr;
    }

    void CallRegistry::remove_call(const std::string& call_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_calls.erase(call_id);

        common::Logger::instance().log(
            "Proxy",
            call_id,
            "TERMINATED",
            "Call session removed"
        );
    }

}