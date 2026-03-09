//
// Created by lazarstani on 3/6/26.
//

#include "../include/networking/udp_transport.h"
#include <chrono>
#include <thread>
#include <iostream>

UdpTransport::UdpTransport() {}

UdpTransport::~UdpTransport() {
    stop();
}

void UdpTransport::start(uint16_t port,
                         ReceiveCallback cb) {
    if (running)
        return;

    callback = cb;

    if (!socket.bind(port)) {
        std::cerr << "Failed to bind UDP socket\n";
        return;
    }

    running = true;

    recv_thread = std::thread(&UdpTransport::receiveLoop, this);
}

void UdpTransport::send(const std::string& data,
                        const std::string& ip,
                        uint16_t port) {

    socket.sendTo(data, ip, port);
}

void UdpTransport::stop() {

    running = false;

    if (recv_thread.joinable())
        recv_thread.join();
}

void UdpTransport::receiveLoop() {

    char buffer[65536];

    while (running) {

        std::string ip;
        uint16_t port;

        ssize_t received =
            socket.recvFrom(buffer, sizeof(buffer), ip, port);

        if (received > 0) {

            std::string data(buffer, received);

            if (callback)
                callback(data, ip, port);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}