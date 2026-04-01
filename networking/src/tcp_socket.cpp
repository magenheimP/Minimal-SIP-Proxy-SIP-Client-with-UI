//
// Created by vladimirsubotic on 3/27/26.
//

#include "../include/networking/tcp_socket.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdio>

TcpSocket::TcpSocket()
    : socket_fd_(-1)
{
    socket_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        perror("TcpSocket: socket");
        return;
    }

    int opt = 1;
    ::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

TcpSocket::~TcpSocket()
{
    if (socket_fd_ >= 0)
        ::close(socket_fd_);
}

bool TcpSocket::bind(uint16_t port)
{
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return ::bind(socket_fd_,
                  reinterpret_cast<sockaddr*>(&addr),
                  sizeof(addr)) == 0;
}

bool TcpSocket::listen(int backlog)
{
    return ::listen(socket_fd_, backlog) == 0;
}

int TcpSocket::accept(std::string& client_ip, uint16_t& client_port)
{
    sockaddr_in addr{};
    socklen_t   len = sizeof(addr);

    int client_fd = ::accept(socket_fd_,
                             reinterpret_cast<sockaddr*>(&addr),
                             &len);
    if (client_fd < 0)
        return -1;

    char ipbuf[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf));
    client_ip   = ipbuf;
    client_port = ntohs(addr.sin_port);

    return client_fd;
}

bool TcpSocket::connect(const std::string& server_ip, uint16_t server_port)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(server_port);

    if (::inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr) <= 0) {
        perror("TcpSocket: inet_pton");
        return false;
    }
    return ::connect(socket_fd_,
                     reinterpret_cast<sockaddr*>(&addr),
                     sizeof(addr)) == 0;
}

ssize_t TcpSocket::sendToFd(int fd, const std::string& data)
{
    return ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);
}

ssize_t TcpSocket::recvFromFd(int fd, char* buffer, size_t size)
{
    return ::recv(fd, buffer, size, 0);
}

void TcpSocket::closeClient(int client_fd)
{
    if (client_fd >= 0)
        ::close(client_fd);
}

int TcpSocket::fd() const
{
    return socket_fd_;
}

uint16_t TcpSocket::local_port() const
{
    sockaddr_in addr{};
    socklen_t   len = sizeof(addr);
    ::getsockname(socket_fd_,
                  reinterpret_cast<sockaddr*>(&addr),
                  &len);
    return ntohs(addr.sin_port);
}
