#include "client/sip_client.hpp"

#include <iostream>
#include <string>
#include <limits>
#include <exception>

int main() {
    std::string server_ip = "127.0.0.1";
    int server_port = 5060;

    try {
        SIPClient client(server_ip, server_port);
        bool running = true;
        while (running) {
            std::cout << "> ";
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "register") {
                std::string username;
                std::string domain;

                std::cout << "username: ";
                if (!(std::cin >> username)) {
                    std::cerr << "Failed to read username.\n";
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    continue;
                }

                std::cout << "domain: ";
                if (!(std::cin >> domain)) {
                    std::cerr << "Failed to read domain.\n";
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    continue;
                }

                if (username.empty() || domain.empty()) {
                    std::cerr << "Username and domain cannot be empty.\n";
                    continue;
                }

                try {
                    //std::string message = client.buildRegisterMessage(username, domain);
                    //std::cout << message << "\n";

                     client.sendRegister(username, domain);
                }
                catch (const std::exception& e) {
                    std::cerr << "REGISTER error: " << e.what() << "\n";
                    std::cerr << "Try again or check server connection.\n";
                }
            }
            else if (cmd == "exit" || cmd== "quit") {
                std::cout << "Exiting client.\n";
                running = false;
            }
            else {
                std::cout << "Unknown command: " << cmd << "\n";
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error starting SIP client: " << e.what() << "\n";
        return 1;
    }

    return 0;
}