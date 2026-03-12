#pragma once
#include <string>
#include "common/sip_message.hpp"
#include "networking/udp_transport.hpp"

class SIPClient {
public:
    SIPClient(const std::string& ip, int port);

    std::string buildRegisterMessage(const std::string& username, const std::string& domain);
    void sendRegister(UdpTransport& transport,
                  const std::string& username,
                  const std::string& domain,
                  bool verbose = true);
    std::string serializeMessage(const common::SIPMessage& msg);

private:
    std::string server_ip;
    int server_port;
    static int global_cseq;
    static std::string global_call_id;
};
