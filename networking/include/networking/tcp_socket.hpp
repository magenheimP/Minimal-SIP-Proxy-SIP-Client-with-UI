//
// Created by vladimirsubotic on 3/27/26.
//

#ifndef _TCP_SOCKET_HPP
#define _TCP_SOCKET_HPP

#pragma once

#include <string>
#include <cstdint>
#include <netinet/in.h>

using namespace std;

class TcpSocket{
public:
    TcpSocket();
    ~TcpSocket();

    bool bind(uint16_t port);
    bool listen(int backlog = 10);

    int accept(string& client_ip, uint16_t& client_port);

    bool connect(const string& server_ip, uint16_t server_port);

    ssize_t sendToFd(int fd, const string& data);
    ssize_t recvFromFd(int fd, char* buffer, size_t size);

    void closeClient(int client_fd);

    int fd() const;
    uint16_t local_port() const;

private:
    int socket_fd_;
};


#endif //_TCP_SOCKET_HPP