#include "proxy/call_session.hpp"

namespace proxy
{

    CallSession::CallSession(std::string call_id,
                             std::string caller,
                             std::string callee)
        : m_call_id(std::move(call_id)),
          m_caller(std::move(caller)),
          m_callee(std::move(callee))
    {
    }

    void CallSession::handle_event(CallEvent event)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        CallState old_state = m_state;

       CallState new_state = CallStateMachine::handle_event(m_state, event);

        if (old_state != new_state)
        {
            m_state = new_state;

            common::Logger::instance().log(
                "Proxy",
                m_call_id,
                to_string(new_state),
                "State transition " +
                to_string(old_state)     +
                " -> " +
                to_string(new_state)
            );
        }
    }

    CallState CallSession::state() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state;
    }

    const std::string& CallSession::call_id() const
    {
        return m_call_id;
    }

}