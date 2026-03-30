//
// Created by lazarstani on 3/30/26.
//

#ifndef SIPTRAININGPROJECT_TCP_TRANSPORT_HPP
#define SIPTRAININGPROJECT_TCP_TRANSPORT_HPP
#pragma once

#include "IUdpTransport.hpp"
#include "tcp_socket.hpp"

#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

class TcpTransport : public IUdpTransport {
    TcpTransport();
    ~TcpTransport();

    void start(uint16_t port, ReceiveCallback callback) override;
    void stop() override;
    void connect(const std::string& server_ip,
                 uint16_t           server_port,
                 ReceiveCallback    callback);
    void send(const std::string& data,
              const std::string& remote_ip,
              uint16_t           remote_port) override;
    uint16_t local_port() const;

private:
    void acceptLoop();
    void receiveLoop(int client_fd, std::string ip, uint16_t port);
    std::string connectionKey(const std::string& ip, uint16_t port) const;

    TcpSocket          socket_;
    ReceiveCallback    callback_;
    std::atomic<bool>  running_{false};
    std::thread accept_thread_;
    std::mutex recv_threads_mutex_;
    std::vector<std::thread> recv_threads_;
    std::mutex connections_mutex_;
    std::unordered_map<std::string, int> connections_;
    bool client_mode_{false};
    int  client_fd_{-1};
};
#endif //SIPTRAININGPROJECT_TCP_TRANSPORT_HPP
