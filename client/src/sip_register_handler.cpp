//
// Created by aleksa on 3/10/26.
//
#include "client/sip_register_handler.hpp"
#include "client/sip_client.hpp"
#include "networking/udp_transport.hpp"
#include "client/sip_receive_handler.hpp"
#include "common/logger.hpp"

#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

SIPRegisterHandler::SIPRegisterHandler(SIPClient& client,
                                       UdpTransport& transport,
                                       SIPReceiveHandler& receiver,
                                       common::Logger& logger)
    : client_(client), transport_(transport), receiver_(receiver), logger_(logger)
{
}

void SIPRegisterHandler::handle_register(const std::string& username, const std::string& domain)
{
    logger_.log("CLIENT", "-", "REGISTER_START",
                "User=" + username + " Domain=" + domain);

    receiver_.reset(); // reset before starting attempts

    for (int i = 0; i < 5; ++i) {
        logger_.log("CLIENT", "-", "REGISTER_ATTEMPT", "Attempt " + std::to_string(i+1));

        if (i == 0)
            client_.sendRegister(transport_, username, domain, true);
        else {
            std::cout << "Retrying REGISTER...\n";
            client_.sendRegister(transport_, username, domain, false);
        }

        bool received = receiver_.wait_for_response(2); // wait up to 2 seconds
        if (received) break; // stop retrying if got response
    }

    // Use local copy to avoid race conditions
    bool received;
    std::string response;
    {
        std::lock_guard<std::mutex> lock(receiver_.mtx);
        received = receiver_.register_response_received;
        response = receiver_.register_response;
    }

    if (!received) {
        logger_.log("CLIENT", "-", "REGISTER_TIMEOUT", "No response from server after retries");
        std::cout << "REGISTER timed out\n";
        return;
    }

    if (response.find("SIP/2.0 200 OK") != std::string::npos) {
        logger_.log("CLIENT", "-", "REGISTER_SUCCESS", "Server returned 200 OK");
        std::cout << "REGISTER successful.\n";
    } else if (response.find("SIP/2.0 401") != std::string::npos) {
        logger_.log("CLIENT", "-", "REGISTER_FAIL", "401 Unauthorized");
        std::cout << "REGISTER failed: 401 Unauthorized\n";
    } else {
        logger_.log("CLIENT", "-", "REGISTER_UNKNOWN_RESPONSE", response);
        std::cout << "Unexpected response.\n";
    }
}