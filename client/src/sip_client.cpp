#include "client/sip_client.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <ctime>


#include "common/sip_message.hpp"
#include "../include/networking/udp_transport.hpp"


int SIPClient::global_cseq = 1;
std::string SIPClient::global_call_id;

SIPClient::SIPClient(const std::string& ip, int port)
    : server_ip(ip), server_port(port)
{
    std::srand(std::time(nullptr));
    global_call_id = std::to_string(std::rand() % 1000000) + "@" + server_ip;
}
std::string SIPClient::serializeMessage(const common::SIPMessage& msg)
{
    std::ostringstream oss;
    oss << msg.start_line << "\r\n";
    for (const auto& h : msg.headers)
    {
        oss << h.name << ": " << h.value << "\r\n";
    }
    oss << "\r\n";
    return oss.str();
}
std::string SIPClient::buildRegisterMessage(const std::string& username, const std::string& domain)
{
    common::SIPMessage msg;
    msg.start_line = "REGISTER sip:" + domain + " SIP/2.0";

    std::string port_str = std::to_string(server_port);

    msg.add_header("Via", "SIP/2.0/UDP " + server_ip + ":" + port_str);
    msg.add_header("From", "<sip:" + username + "@" + domain + ">");
    msg.add_header("To", "<sip:" + username + "@" + domain + ">");
    msg.add_header("Call-ID", global_call_id);
    msg.add_header("CSeq", std::to_string(global_cseq) + " REGISTER");
    msg.add_header("Contact", "<sip:" + username + "@" + server_ip + ">");
    msg.add_header("Content-Length", "0");

    global_cseq++;


    return serializeMessage(msg);
}



void SIPClient::sendRegister(UdpTransport& transport,
                             const std::string& username,
                             const std::string& domain,
                             bool verbose)
{
    std::string message = buildRegisterMessage(username, domain);

    transport.send(message, server_ip, server_port);

    if (verbose) {
        std::cout << "Sent REGISTER:\n" << message << "\n";
    }
}