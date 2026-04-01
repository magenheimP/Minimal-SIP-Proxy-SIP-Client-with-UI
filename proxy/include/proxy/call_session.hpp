#pragma once

#include <string>
#include <mutex>
#include "call_state_machine.hpp"
#include "call_state.hpp"
#include "common/logger.hpp"
#include "common/sip_message.hpp"

namespace proxy
{

    class CallSession
    {
    public:
        CallSession(std::string call_id,
                    std::string caller,
                    std::string callee);


        void on_request(const common::SIPMessage& message);


        void on_response(const common::SIPMessage& message);

        CallState state() const;
        const std::string& call_id() const;

        bool is_terminated() const;

    private:
        void apply_event(CallEvent event);

        std::string m_call_id;
        std::string m_caller;
        std::string m_callee;

        mutable std::mutex m_mutex;

        CallState m_state {CallState::INIT};
    };

}