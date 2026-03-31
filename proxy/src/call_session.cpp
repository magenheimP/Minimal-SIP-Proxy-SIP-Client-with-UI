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

    void CallSession::on_request(const common::SIPMessage& message)
    {
        // Extract the method from the message and map it to the corresponding
        // internal event. Unknown methods are silently ignored — the router
        // handles 501 responses for unsupported methods.
        const std::string method = message.get_method();

        if (method == "INVITE") {
            apply_event(CallEvent::INVITE);
        } else if (method == "ACK") {
            apply_event(CallEvent::ACK);
        } else if (method == "BYE") {
            apply_event(CallEvent::BYE);
        }
        else {
            common::Logger::instance().log(
                common::LogLevel::ERROR,
                "CallSession",
                m_call_id,
                "IGNORED",
                "Unsupported method: " + method
            );
        }
    }

    void CallSession::on_response(const common::SIPMessage& message)
    {

        const int status_code = message.get_status_code();
        const std::string cseq = message.get_header("CSeq");

        if (status_code == 100) {
            apply_event(CallEvent::TRYING_100);
        } else if (status_code == 180) {
            apply_event(CallEvent::RINGING_180);
        } else if (status_code == 200 && cseq.find("INVITE") != std::string::npos) {
            apply_event(CallEvent::OK_200);
        }

    }

    void CallSession::apply_event(CallEvent event)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        CallState old_state = m_state;
        CallState new_state = CallStateMachine::handle_event(m_state, event);

        if (old_state == new_state) {
            common::Logger::instance().log(
                common::LogLevel::INFO,
                "CallSession",
                m_call_id,
                "STATE_UNCHANGED",
                "Event had no state transition from " + to_string(old_state)
            );
        }

        if (old_state != new_state)
        {
            m_state = new_state;

            common::Logger::instance().log(
                "Proxy",
                m_call_id,
                to_string(new_state),
                "State transition " +
                to_string(old_state) +
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
    bool CallSession::is_terminated() const { return state() == CallState::TERMINATED; }

}