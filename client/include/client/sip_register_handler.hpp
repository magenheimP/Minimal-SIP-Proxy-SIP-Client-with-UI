#pragma once
#include <string>

class SIPClient;
class UdpTransport;
class SIPReceiveHandler;

namespace common {
    class Logger;
}

class SIPRegisterHandler {
public:
    SIPRegisterHandler(SIPClient& client,
                       UdpTransport& transport,
                       SIPReceiveHandler& receiver,
                       common::Logger& logger);

    void handle_register(const std::string& username, const std::string& domain);

private:
    SIPClient& client_;
    UdpTransport& transport_;
    SIPReceiveHandler& receiver_;
    common::Logger& logger_;
};