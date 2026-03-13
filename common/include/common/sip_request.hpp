#pragma once
#include <string>

namespace common {

    struct SIPRequest {
        enum class Method { REGISTER, INVITE, BYE, ACK } method;
        std::string from_username;
        std::string from_domain;
        std::string to_username;
        std::string to_domain;
        std::string call_id;
        std::string contact;
        std::string body;

    };

}