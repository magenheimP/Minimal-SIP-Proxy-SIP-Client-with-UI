//
// Created by lazarstani on 3/6/26.
//

#ifndef SIPTRAININGPROJECT_UDP_SOCKET_H
#define SIPTRAININGPROJECT_UDP_SOCKET_H
#prragma once
#include <string>
#include <netinet/in.h>

class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    bool bind(uint16_t port);

    ssize_t sendTo(
        const std::string& data,
        const std::string& ip,
        uint16_t port);

    ssize_t recvFrom(
        char* buffer,
        size_t size,
        std::string& ip,
        uint16_t& port);

    int fd() const;

private:
    int socket_fd;
}
#endif //SIPTRAININGPROJECT_UDP_SOCKET_H