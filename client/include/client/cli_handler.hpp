#pragma once
#include <string>

class SIPClient;

class CLIHandler {
public:
    explicit CLIHandler(SIPClient& client);

    void run();

private:
    bool prompt(const std::string& prompt_text, std::string& out) const;

    void handle_register_command();

    SIPClient& client_;
};
