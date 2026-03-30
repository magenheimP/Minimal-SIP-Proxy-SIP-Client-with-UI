//
// Created by lazarstani on 3/30/26.
//

#include "networking/tcp_transport.hpp"

#include <iostream>
#include <unistd.h>


TcpTransport::TcpTransport() = default;

TcpTransport::~TcpTransport()
{
    stop();
}

void TcpTransport::start(uint16_t port, ReceiveCallback callback)
{
    if (running_)
        return;

    callback_     = std::move(callback);
    client_mode_  = false;

    if (!socket_.bind(port))
        throw std::runtime_error("TcpTransport: bind failed on port "
                                 + std::to_string(port));

    if (!socket_.listen())
        throw std::runtime_error("TcpTransport: listen failed");

    running_ = true;

    accept_thread_ = std::thread(&TcpTransport::acceptLoop, this);
}

void TcpTransport::stop()
{
    if (!running_)
        return;

    running_ = false;

    ::shutdown(socket_.fd(), SHUT_RDWR);

    if (accept_thread_.joinable())
        accept_thread_.join();

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& [key, fd] : connections_)
            socket_.closeClient(fd);
        connections_.clear();
    }

    std::lock_guard<std::mutex> lock(recv_threads_mutex_);
    for (auto& t : recv_threads_)
        if (t.joinable())
            t.join();

    recv_threads_.clear();
}

void TcpTransport::connect(const std::string& server_ip,
                            uint16_t           server_port,
                            ReceiveCallback    callback)
{
    if (running_)
        return;

    callback_    = std::move(callback);
    client_mode_ = true;

    if (!socket_.connect(server_ip, server_port))
        throw std::runtime_error(
            "TcpTransport: connect failed to "
            + server_ip + ":" + std::to_string(server_port));

    client_fd_ = socket_.fd();
    running_   = true;

    std::lock_guard<std::mutex> lock(recv_threads_mutex_);
    recv_threads_.emplace_back(
        &TcpTransport::receiveLoop,
        this,
        client_fd_,
        server_ip,
        server_port);
}

uint16_t TcpTransport::local_port() const
{
    return socket_.local_port();
}

void TcpTransport::send(const std::string& data,
                        const std::string& remote_ip,
                        uint16_t           remote_port)
{
    if (client_mode_) {
        if (client_fd_ >= 0)
            socket_.sendToFd(client_fd_, data);
        return;
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    const auto it = connections_.find(connectionKey(remote_ip, remote_port));
    if (it == connections_.end()) {
        std::cerr << "TcpTransport: no connection for "
                  << remote_ip << ":" << remote_port << "\n";
        return;
    }

    socket_.sendToFd(it->second, data);
}

void TcpTransport::acceptLoop()
{
    while (running_) {
        std::string client_ip;
        uint16_t    client_port = 0;

        int client_fd = socket_.accept(client_ip, client_port);

        if (!running_)
            break;

        if (client_fd < 0) {
            perror("TcpTransport: accept");
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_[connectionKey(client_ip, client_port)] = client_fd;
        }

        {
            std::lock_guard<std::mutex> lock(recv_threads_mutex_);
            recv_threads_.emplace_back(
                &TcpTransport::receiveLoop,
                this,
                client_fd,
                client_ip,
                client_port);
        }
    }
}

void TcpTransport::receiveLoop(int client_fd, std::string ip, uint16_t port)
{
    constexpr size_t BUFFER_SIZE = 4096;
    char             buf[BUFFER_SIZE];
    std::string      accumulator;

    while (running_) {
        ssize_t n = socket_.recvFromFd(client_fd, buf, BUFFER_SIZE);

        if (n <= 0) {
            break;
        }

        accumulator.append(buf, static_cast<size_t>(n));

        size_t pos;
        while ((pos = accumulator.find("\r\n\r\n")) != std::string::npos) {
            std::string message = accumulator.substr(0, pos + 4);
            accumulator.erase(0, pos + 4);

            if (callback_)
                callback_(message, ip, port);
        }
    }

    if (!client_mode_) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(connectionKey(ip, port));
        socket_.closeClient(client_fd);
    }
}

std::string TcpTransport::connectionKey(const std::string& ip,
                                         uint16_t           port) const
{
    return ip + ":" + std::to_string(port);
}