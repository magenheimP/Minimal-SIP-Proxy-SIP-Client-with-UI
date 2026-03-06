#include "client/sip_client.hpp"
#include <iostream>

int main() {

   // SipClient client;

    while(true) {

        std::string cmd;
        std::cout << "> ";
        std::cin >> cmd;

        if(cmd == "register") {

            std::string username, domain, ip;
            int port;

            std::cout << "username: ";
            std::cin >> username;

            std::cout << "domain: ";
            std::cin >> domain;

            //client.sendRegister(username, domain);
        }

        if(cmd == "exit")
            break;
    }
}