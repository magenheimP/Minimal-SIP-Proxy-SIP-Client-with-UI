#include "client/sip_client.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static int global_cseq = 1;
static std::string global_call_id;

SIPClient::SIPClient(const std::string& ip, int port)
    : server_ip(ip), server_port(port)
{
    std::srand(std::time(nullptr));
    global_call_id = std::to_string(std::rand() % 1000000) + "@" + server_ip;
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

    std::ostringstream oss;

    oss << msg.start_line << "\r\n";

    for (const auto& h : msg.headers)
    {
        oss << h.name << ": " << h.value << "\r\n";
    }

    oss << "\r\n";

    global_cseq++;

    return oss.str();
}

void SIPClient::sendRegister(const std::string& username, const std::string& domain)
{
    std::string message = buildRegisterMessage(username, domain);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid server IP address\n";
        close(sock);
        return;
    }

    ssize_t sent = sendto(
        sock,
        message.c_str(),
        message.size(),
        0,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    );

    if (sent < 0)
    {
        perror("Failed to send REGISTER");
        close(sock);
        return;
    }

    std::cout << "Sent REGISTER:\n" << message << "\n";

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeval timeout{};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ready = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

    if (ready < 0)
    {
        perror("select() failed");
        close(sock);
        return;
    }

    if (ready == 0)
    {
        std::cerr << "No response from server (timeout)\n";
        close(sock);
        return;
    }

    char buffer[2048];

    sockaddr_in from_addr{};
    socklen_t from_len = sizeof(from_addr);

    ssize_t recv_len = recvfrom(
        sock,
        buffer,
        sizeof(buffer) - 1,
        0,
        (struct sockaddr*)&from_addr,
        &from_len
    );

    if (recv_len < 0)
    {
        perror("Failed to receive response");
        close(sock);
        return;
    }

    buffer[recv_len] = '\0';

    std::string response(buffer);

    std::cout << "Received response:\n" << response << "\n";

    if (response.find("SIP/2.0 200 OK") != std::string::npos)
    {
        std::cout << "REGISTER successful\n";
    }
    else if (response.find("SIP/2.0 401") != std::string::npos)
    {
        std::cerr << "REGISTER failed: 401 Unauthorized\n";
        std::cerr << "Server requires authentication\n";
    }
    else if (response.find("SIP/2.0") != std::string::npos)
    {
        std::cerr << "Server returned SIP error\n";
    }
    else
    {
        std::cerr << "Received non-SIP response\n";
    }

    close(sock);
}