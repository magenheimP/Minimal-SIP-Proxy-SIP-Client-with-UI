#include "client/sip_client.hpp"
#include <iostream>

int main() {
    std::string server_ip = "127.0.0.1";
    int server_port = 5060;

    SIPClient client(server_ip, server_port);

    while (true) {
        std::string cmd;
        std::cout << "> ";
        std::cin >> cmd;

        if (cmd == "register") {
            std::string username, domain;
            std::cout << "username: ";
            std::cin >> username;
            std::cout << "domain: ";
            std::cin >> domain;

            //client.sendRegister(username, domain);
            std::string message=client.buildRegisterMessage(username, domain);
            std::cout << message << std::endl;
        }

        if (cmd == "exit")
            break;
    }
}