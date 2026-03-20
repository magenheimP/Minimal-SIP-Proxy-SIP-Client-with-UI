#include "client/sip_register_handler.hpp"
#include "client/sip_client.hpp"
#include "client/sip_client_state_manager.hpp"

#include <iostream>

SIPRegisterHandler::SIPRegisterHandler(SIPClient& client)
    : client_(client)
{}

void SIPRegisterHandler::handle_register(const std::string& username,
                                          const std::string& domain)
{
    auto& log = client_.logger();
    log.log("CLIENT", "-", "REGISTER_START",
            "User=" + username + " Domain=" + domain);

    client_.state().on_registering();
    client_.reset_receive_state();

    const std::string message = client_.build_register_message(username, domain);

    constexpr int max_attempts = 5;
    constexpr int timeout_sec  = 2;

    for (int i = 0; i < max_attempts; ++i) {
        log.log("CLIENT", "-", "REGISTER_ATTEMPT",
                "Attempt " + std::to_string(i + 1));

        if (i == 0)
            std::cout << "Sent REGISTER:\n" << message << "\n";
        else
            std::cout << "Retrying REGISTER...\n";

        client_.send_to_server(message);

        if (client_.wait_for_register_response(timeout_sec))
            break;
    }

    auto [received, response] = client_.register_response_snapshot();

    if (!received) {
        log.log("CLIENT", "-", "REGISTER_TIMEOUT",
                "No response from server after retries");
        std::cout << "REGISTER timed out.\n";
        client_.state().on_register_failed();
        return;
    }

    if (response.find("SIP/2.0 200 OK") != std::string::npos) {
        log.log("CLIENT", "-", "REGISTER_SUCCESS", "Server returned 200 OK");
        std::cout << "REGISTER successful.\n";
        client_.state().on_register_success(username, domain);

    } else if (response.find("SIP/2.0 401") != std::string::npos) {
        log.log("CLIENT", "-", "REGISTER_FAIL", "401 Unauthorized");
        std::cout << "REGISTER failed: 401 Unauthorized.\n";
        client_.state().on_register_failed();

    } else {
        log.log("CLIENT", "-", "REGISTER_UNKNOWN_RESPONSE", response);
        std::cout << "Unexpected response.\n";
        client_.state().on_register_failed();
    }
}