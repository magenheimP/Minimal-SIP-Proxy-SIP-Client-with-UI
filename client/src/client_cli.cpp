#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <iostream>
#include <thread>

#include "client/sip_client.hpp"
#include "common/logger.hpp"

std::mutex mtx;
std::condition_variable cv;
bool register_response_received = false;
std::string register_response;

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

int main() {
    std::string server_ip = "127.0.0.1";
    int server_port = 5060;

    auto& logger = common::Logger::instance("client.log");

    logger.log("CLIENT", "-", "STARTUP", "SIP client starting");

    try {
        SIPClient client(server_ip, server_port);
        UdpTransport transport;

        logger.log("NETWORK", "-", "INIT", "UDP transport initializing");

        transport.start(
            0,
            [&](const std::string& data,
                const std::string& sender_ip,
                uint16_t sender_port)
            {
                logger.log(
                    "NETWORK",
                    "-",
                    "RECV_PACKET",
                    "Received packet from " + sender_ip + ":" + std::to_string(sender_port)
                );

                logger.log("NETWORK", "-", "SIP_MESSAGE", data);

                std::cout << "\nReceived SIP response from "
                          << sender_ip << ":" << sender_port << "\n";
                std::cout << data << "\n";

                if (data.find("SIP/2.0") != std::string::npos) {
                    std::lock_guard<std::mutex> lock(mtx);
                    register_response_received = true;
                    register_response = data;

                    logger.log("CLIENT", "-", "REGISTER_RESPONSE", "Response received");

                    cv.notify_one();
                }

                std::cout << "> " << std::flush;
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

                if (!get_input("username: ", username))
                    continue;

                if (!get_input("domain: ", domain))
                    continue;

                logger.log(
                    "CLIENT",
                    "-",
                    "REGISTER_START",
                    "User=" + username + " Domain=" + domain
                );

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    register_response_received = false;
                }

                for (int i = 0; i < 5; i++) {

                    logger.log(
                        "CLIENT",
                        "-",
                        "REGISTER_ATTEMPT",
                        "Attempt " + std::to_string(i + 1)
                    );

                    if (i == 0) {
                        client.sendRegister(transport, username, domain, true);
                    } else {
                        std::cout << "Retrying REGISTER...\n";
                        client.sendRegister(transport, username, domain, false);
                    }

                    std::unique_lock<std::mutex> lock(mtx);

                    if (cv.wait_for(
                            lock,
                            std::chrono::seconds(2),
                            [] { return register_response_received; }))
                    {
                        logger.log(
                            "CLIENT",
                            "-",
                            "REGISTER_RESPONSE_RECEIVED",
                            "Response received during attempt " + std::to_string(i + 1)
                        );
                        break;
                    }
                }

                bool received;

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    received = register_response_received;
                }

                if (!received) {

                    logger.log(
                        "CLIENT",
                        "-",
                        "REGISTER_TIMEOUT",
                        "No response from server after retries"
                    );

                    std::cout << "REGISTER timed out\n";
                }
                else {

                    if (register_response.find("SIP/2.0 200 OK") != std::string::npos) {

                        logger.log(
                            "CLIENT",
                            "-",
                            "REGISTER_SUCCESS",
                            "Server returned 200 OK"
                        );

                        std::cout << "REGISTER successful.\n";
                    }
                    else if (register_response.find("SIP/2.0 401") != std::string::npos) {

                        logger.log(
                            "CLIENT",
                            "-",
                            "REGISTER_FAIL",
                            "401 Unauthorized"
                        );

                        std::cout << "REGISTER failed: 401 Unauthorized\n";
                    }
                    else {

                        logger.log(
                            "CLIENT",
                            "-",
                            "REGISTER_UNKNOWN_RESPONSE",
                            register_response
                        );

                        std::cout << "Unexpected response.\n";
                    }
                }
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

        logger.log(
            "CLIENT",
            "-",
            "FATAL",
            std::string("Fatal error: ") + e.what()
        );

        std::cerr << "Fatal error starting SIP client: " << e.what() << "\n";
        return 1;
    }

    logger.log("CLIENT", "-", "EXIT", "Program terminated");

    return 0;
}