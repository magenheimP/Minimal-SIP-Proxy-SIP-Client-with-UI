//
// Created by lazarstani on 3/6/26.
//
#include "udp_socket.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

UdpSocket::UdpSocket() {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    int flags = fcntl(socket_fd, F_GETFL,0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

UdpSocket::~UdpSocket() {
    if (socket_fd >= 0) close(socket_fd);
}

bool UdpSocket::bind(uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    return ::bind(socket_fd,
        reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

ssize_t UdpSocket::sendTo(
    const std::string& data,
    const std::string& ip,
    uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    inet_pton(AF_INET, ip.c_str(), &address.sin_addr);

    return sendTo(socket_fd,
        data.data(),
        data.size(),
        0,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address));
}

ssize_t UdpSocket::recvFrom(char* buffer,
                            size_t size,
                            std::string& ip,
                            uint16_t& port) {
    sockaddr_in address{};
    socklen_t len = sizeof(address);

    ssize_t received = recvfrom(socket_fd,
                                buffer,
                                size,
                                0,
                                (sockaddr*)&addr,
                                &len);

    if (received > 0) {
        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf));
        ip = ipbuf;
        port = ntohs(addr.sin_port);
    }

    return received;
}

int UdpSocket::fd() const {
    return socket_fd;
}