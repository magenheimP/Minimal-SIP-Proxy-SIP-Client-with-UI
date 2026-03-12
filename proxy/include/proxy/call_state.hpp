#pragma once

#include <string>

namespace proxy {

    enum class CallState
    {
        INIT,
        TRYING,
        RINGING,
        ESTABLISHED,
        TERMINATED
    };

    const std::string to_string(CallState state);

}