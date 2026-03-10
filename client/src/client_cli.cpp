#include <iostream>
#include <string>
#include "client/sip_client.hpp"
#include "networking/udp_transport.hpp"
#include "common/logger.hpp"
#include "client/sip_register_handler.hpp"
#include "client/input_handler.hpp"
#include "client/sip_receive_handler.hpp"
#include "client/sip_register_handler.hpp"

int main() {
    std::string server_ip = "127.0.0.1";
    int server_port = 5060;

    auto& logger = common::Logger::instance("client.log");
    logger.log("CLIENT", "-", "STARTUP", "SIP client starting");

    try {
        SIPClient client(server_ip, server_port);
        UdpTransport transport;

        SIPReceiveHandler receiver;
        InputHandler input_handler;
        SIPRegisterHandler register_handler(client, transport, receiver, logger);

        transport.start(
            0,
            [&](const std::string& data,
                const std::string& sender_ip,
                uint16_t sender_port)
            {
                logger.log("NETWORK", "-", "RECV_PACKET",
                           "Received packet from " + sender_ip + ":" + std::to_string(sender_port));
                logger.log("NETWORK", "-", "SIP_MESSAGE", data);

                std::cout << "\nReceived SIP response from "
                          << sender_ip << ":" << sender_port << "\n";
                std::cout << data << "\n> " << std::flush;

                receiver.handle_receive(data);
            }
        );

        logger.log("NETWORK", "-", "READY", "UDP transport started");

        bool running = true;

        while (running) {
            std::cout << "> " << std::flush;
            std::string cmd;
            std::cin >> cmd;

            logger.log("CLI", "-", "COMMAND", cmd);

            if (cmd == "register") {
                std::string username, domain;
                if (!input_handler.get_input("username: ", username))
                    continue;
                if (!input_handler.get_input("domain: ", domain))
                    continue;

                register_handler.handle_register(username, domain);
            }
            else if (cmd == "exit" || cmd == "quit") {
                logger.log("CLIENT", "-", "SHUTDOWN", "Client exiting");
                std::cout << "Exiting client.\n";
                running = false;
            }
            else {
                logger.log("CLI", "-", "UNKNOWN_COMMAND", cmd);
                std::cout << "Unknown command: " << cmd << "\n";
            }
        }

        transport.stop();
        logger.log("NETWORK", "-", "STOP", "UDP transport stopped");
    }
    catch (const std::exception& e) {
        logger.log("CLIENT", "-", "FATAL", std::string("Fatal error: ") + e.what());
        std::cerr << "Fatal error starting SIP client: " << e.what() << "\n";
        return 1;
    }

    logger.log("CLIENT", "-", "EXIT", "Program terminated");
    return 0;
}
