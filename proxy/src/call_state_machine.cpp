#include "proxy/call_state_machine.hpp"

namespace proxy {

    CallState CallStateMachine::handle_event(const CallState current, const CallEvent event)
    {
        switch (current)
        {
            case CallState::INIT:
                if (event == CallEvent::INVITE)
                    return CallState::TRYING;
                break;

            case CallState::TRYING:
                if (event == CallEvent::TRYING_100)
                    return CallState::TRYING;

                if (event == CallEvent::RINGING_180)
                    return CallState::RINGING;
                break;

            case CallState::RINGING:
                if (event == CallEvent::OK_200)
                    return CallState::ESTABLISHED;
                break;

            case CallState::ESTABLISHED:
                if (event == CallEvent::ACK)
                    return CallState::ESTABLISHED;

                if (event == CallEvent::BYE)
                    return CallState::TERMINATED;
                break;

            case CallState::TERMINATED:
                break;
        }

        return current; // invalid transition → stay in same state
    }

}