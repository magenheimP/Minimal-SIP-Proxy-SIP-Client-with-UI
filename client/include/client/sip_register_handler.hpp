#pragma once
#include <string>

class SIPClient;


class SIPRegisterHandler {
public:
    explicit SIPRegisterHandler(SIPClient& client);

    void handle_register(const std::string& username,
                         const std::string& domain);

private:
    SIPClient& client_;
};
