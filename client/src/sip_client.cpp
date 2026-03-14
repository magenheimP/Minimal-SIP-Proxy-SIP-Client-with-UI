#include "client/sip_client.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>\n";
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 5060\n";
        return 1;
    }

    const std::string server_ip   = argv[1];
    const int         server_port = std::stoi(argv[2]);

    try {
        common::Logger::instance("client.log")
               .log("MAIN", "-", "START", "Client starting");
        SIPClient client(server_ip, server_port);
        client.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

SIPClient::SIPClient(const std::string& server_ip, int server_port)
    : server_ip_(server_ip)
    , server_port_(server_port)
    , logger_(common::Logger::instance())
    , transport_()
    , factory_(server_ip, server_port)
    , receiver_()
    , state_()
    , register_handler_(*this)
    , cli_(*this)
{
    logger_.log("CLIENT", "-", "STARTUP", "SIPClient constructed");
}

void SIPClient::run()
{
    transport_.start(
        0,
        [this](const std::string& data,
               const std::string& sender_ip,
               uint16_t           sender_port)
        {
            on_packet_received(data, sender_ip, sender_port);
        }
    );

    logger_.log("NETWORK", "-", "READY", "UDP transport started");

    cli_.run();

    transport_.stop();
    logger_.log("NETWORK", "-", "STOP", "UDP transport stopped");
    logger_.log("CLIENT", "-", "EXIT",  "Program terminated");
}

void SIPClient::send_to_server(const std::string& message)
{
    transport_.send(message, server_ip_, static_cast<uint16_t>(server_port_));
}

bool SIPClient::wait_for_register_response(int timeout_seconds)
{
    return receiver_.wait_for_response(timeout_seconds);
}

void SIPClient::reset_receive_state()
{
    receiver_.reset();
}

std::pair<bool, std::string> SIPClient::register_response_snapshot() const
{
    return { receiver_.get_register_received(),
             receiver_.get_register_response() };
}

std::string SIPClient::build_register_message(const std::string& username, const std::string& domain)
{
    return factory_.build_register(username, domain);
}

SIPClientStateManager& SIPClient::state()
{
    return state_;
}

common::Logger& SIPClient::logger()
{
    return logger_;
}

void SIPClient::do_register(const std::string& username,
                             const std::string& domain)
{
    register_handler_.handle_register(username, domain);
}

void SIPClient::on_packet_received(const std::string& data,
                                    const std::string& sender_ip,
                                    uint16_t           sender_port)
{
    logger_.log("NETWORK", "-", "RECV_PACKET",
                "From " + sender_ip + ":" + std::to_string(sender_port));
    logger_.log("NETWORK", "-", "SIP_MESSAGE", data);

    std::cout << "\nReceived SIP response from "
              << sender_ip << ":" << sender_port << "\n"
              << data << "\n> " << std::flush;

    receiver_.handle_receive(data);
}