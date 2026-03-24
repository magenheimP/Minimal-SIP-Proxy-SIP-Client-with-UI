#pragma once
#include <string>
#include <sstream>
class SIPClient;

class CLIHandler {
public:
    explicit CLIHandler(SIPClient& client);
    void run();

private:
    bool prompt(const std::string& prompt_text, std::string& out) const;

    void handle_register_command();
    void handle_call_command();
    void handle_sleep_command(std::istringstream& iss);

    SIPClient& client_;
};
