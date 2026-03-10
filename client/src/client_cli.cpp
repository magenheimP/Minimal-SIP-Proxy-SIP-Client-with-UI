#include "client/sip_client.hpp"
#include "networking/udp_transport.hpp"

#include <iostream>
#include <string>
#include <limits>
#include <exception>

bool get_input(const std::string& prompt, std::string& out_value) {
    std::cout << prompt;
    if (!(std::cin >> out_value)) {
        std::cerr << "Failed to read input.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }
    if (out_value.empty()) {
        std::cerr << "Input cannot be empty.\n";
        return false;
    }
    return true;
}

void handleRegister(SIPClient& client, UdpTransport& transport) {
    std::string username, domain;

    if (!get_input("username: ", username)) return;
    if (!get_input("domain: ", domain)) return;

    try {

        client.sendRegister(transport, username, domain);

    } catch (const std::exception& e) {
        std::cerr << "REGISTER error: " << e.what() << "\n";
    }
}

int main() {
    std::string server_ip = "127.0.0.1";
    int server_port = 5060;

    try {
        SIPClient client(server_ip, server_port);
        UdpTransport transport;

        transport.start(0, [&](const std::string& data,
        const std::string& sender_ip,uint16_t sender_port)
        {
            std::cout << "\nReceived SIP response from "
             << sender_ip << ":" << sender_port << "\n";
            std::cout << "\nReceived SIP response:\n" << data << "\n";

            if (data.find("SIP/2.0 200 OK") != std::string::npos) {
                std::cout << "REGISTER successful.\n";
            }
            else if (data.find("SIP/2.0 401") != std::string::npos) {
                std::cout << "REGISTER failed: 401 Unauthorized\n";
            }
            else {
                std::cout << "Unexpected response.\n";
            }

            std::cout << "> " << std::flush;
        });

        bool running = true;

        while (running) {
            std::cout << "> " << std::flush;

            std::string cmd;
            std::cin >> cmd;

            if (cmd == "register") {
                handleRegister(client, transport);
            }
            else if (cmd == "exit" || cmd == "quit") {
                std::cout << "Exiting client.\n";
                running = false;
            }
            else {
                std::cout << "Unknown command: " << cmd << "\n";
            }
        }

        transport.stop();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error starting SIP client: " << e.what() << "\n";
        return 1;
    }

    return 0;
}