#pragma once
#include <string>
#include "common/sip_message.h"

class SIPClient {
public:
    SIPClient(const std::string& server_ip, int server_port);

    void sendRegister(const std::string& username, const std::string& domain);
    std::string buildRegisterMessage(const std::string& username, const std::string& domain);

private:
    std::string server_ip;
    int server_port;


};