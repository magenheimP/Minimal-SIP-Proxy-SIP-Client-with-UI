//
// Created by lazarstani on 3/6/26.
//
#include "../include/networking/udp_socket.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

UdpSocket::UdpSocket() {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0) {
        perror("socket");
        return;
    }

    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

UdpSocket::~UdpSocket() {
    if (socket_fd >= 0)
        close(socket_fd);
}

bool UdpSocket::bind(uint16_t port) {

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return ::bind(socket_fd,
                  (sockaddr*)&addr,
                  sizeof(addr)) == 0;
}

ssize_t UdpSocket::sendTo(const std::string& data,
                          const std::string& ip,
                          uint16_t port) {

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    return sendto(socket_fd,
                  data.data(),
                  data.size(),
                  0,
                  (sockaddr*)&addr,
                  sizeof(addr));
}

ssize_t UdpSocket::recvFrom(char* buffer,
                            size_t size,
                            std::string& ip,
                            uint16_t& port) {

    sockaddr_in addr{};
    socklen_t len = sizeof(addr);

    ssize_t received = recvfrom(socket_fd,
                                buffer,
                                size,
                                0,
                                (sockaddr*)&addr,
                                &len);

    if (received < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            perror("recvfrom");
        return received;
    }

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