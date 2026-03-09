//
// Created by lazarstani on 3/6/26.
//

#ifndef SIPTRAININGPROJECT_UDP_TRANSPORT_H
#define SIPTRAININGPROJECT_UDP_TRANSPORT_H
#pragma once
#include "IUdpTransport.hpp"
#include "udp_socket.hpp"
#include <thread>
#include <atomic>

class UdpTransport : public IUdpTransport {
public:

    UdpTransport();
    ~UdpTransport();

    void start(uint16_t port, ReceiveCallback callback) override;

    void send(const std::string& data,
              const std::string& remote_ip,
              uint16_t remote_port) override;

    void stop() override;

private:

    void receiveLoop();

    UdpSocket socket;
    std::thread recv_thread;

    ReceiveCallback callback;

    std::atomic<bool> running{false};
};
#endif //SIPTRAININGPROJECT_UDP_TRANSPORT_H