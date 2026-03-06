#include "client/sip_client.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

SIPClient::SIPClient(const std::string& ip, int port)
    : server_ip(ip), server_port(port) {}

std::string SIPClient::buildRegisterMessage(const std::string& username, const std::string& domain) {
    common::SIPMessage msg;
    msg.start_line = "REGISTER sip:" + domain + " SIP/2.0";
    std::string port_str = std::to_string(server_port);

    msg.add_header("Via", "SIP/2.0/UDP " + server_ip + ":" + port_str);
    msg.add_header("From", "<sip:" + username + "@" + domain + ">");
    msg.add_header("To", "<sip:" + username + "@" + domain + ">");
    msg.add_header("Call-ID", "123456@" + server_ip);
    msg.add_header("CSeq", "1 REGISTER");
    msg.add_header("Contact", "<sip:" + username + "@" + server_ip + ">");
    msg.add_header("Content-Length", "0");

    //This adds : so it would be like this Via:
    std::ostringstream oss;
    // maybe \r\n will be needed for windows
    oss << msg.start_line << "\n";
    for (const auto& h : msg.headers) {
        oss << h.name << ": " << h.value << "\n";
    }
    oss << "\n";

    return oss.str();
}

//This can be tested after Proxy and Network finish their tasks
void SIPClient::sendRegister(const std::string& username, const std::string& domain) {
    std::string message = buildRegisterMessage(username, domain);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    sendto(sock, message.c_str(), message.size(), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));

    std::cout << "Sent REGISTER:\n" << message << "\n";

    close(sock);
}