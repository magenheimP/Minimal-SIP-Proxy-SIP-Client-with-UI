#pragma once
#include <string>

class SIPClient {
public:
    SIPClient();

    void sendRegister();

private:
    std::string server_ip;
    int server_port;
};