#include "proxy/call_state.hpp"

namespace proxy {

    std::string to_string(const CallState state)
    {
        switch (state)
        {
            case CallState::INIT:
                return "INIT";

            case CallState::TRYING:
                return "TRYING";

            case CallState::RINGING:
                return "RINGING";

            case CallState::ESTABLISHED:
                return "ESTABLISHED";

            case CallState::TERMINATED:
                return "TERMINATED";
        }

        return "UNKNOWN";
    }

}