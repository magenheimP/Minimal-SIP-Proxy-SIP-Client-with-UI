#pragma once

#include "proxy/call_state.hpp"

namespace proxy {

    enum class CallEvent
    {
        INVITE,
        TRYING_100,
        RINGING_180,
        OK_200,
        ACK,
        BYE
    };

    class CallStateMachine
    {
    public:
        static CallState handle_event(CallState current, CallEvent event);
    };

}