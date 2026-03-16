#pragma once

#include <string>
#include "call_state_machine.hpp"
#include "call_state.hpp"
#include "common/logger.hpp"

namespace proxy
{

    class CallSession
    {
    public:
        CallSession(std::string call_id,
                    std::string caller,
                    std::string callee);

        void handle_event(CallEvent event);

        CallState state() const;
        const std::string& call_id() const;

    private:
        std::string m_call_id;
        std::string m_caller;
        std::string m_callee;

        mutable std::mutex m_mutex;

        CallState m_state {CallState::INIT};
    };

}