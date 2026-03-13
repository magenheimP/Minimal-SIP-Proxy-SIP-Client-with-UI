//
// Created by lazarstani on 3/6/26.
//

#include "../include/networking/udp_transport.hpp"
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
        throw std::runtime_error("Failed to bind UDP socket");
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
    size_t buffer_size = 65536;
    auto buffer = std::make_unique<char[]>(buffer_size);

    while (running) {
        std::string ip;
        uint16_t port;

        ssize_t received = socket.recvFrom(buffer.get(), buffer_size, ip, port);

        if (received < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
                perror("recvFrom");
        } else if (static_cast<size_t>(received) == buffer_size) {
            buffer_size *= 2;
            buffer = std::make_unique<char[]>(buffer_size);
        } else if (received > 0) {
            std::string data(buffer.get(), received);
            if (callback)
                callback(data, ip, port);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}