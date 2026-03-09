//
// Created by lazarstani on 3/6/26.
//

#ifndef SIPTRAININGPROJECT_IUDPTRANSPORT_H
#define SIPTRAININGPROJECT_IUDPTRANSPORT_H
#pragma once
#include <functional>
#include <string>
#include <cstdint>

class IUdpTransport {
public:
    using ReceiveCallback =
        std::function<void(const std::string& data,
                           const std::string& remote_ip,
                           uint16_t remote_port)>;

    virtual ~IUdpTransport() = default;

    virtual void start(uint16_t port, ReceiveCallback callback) = 0;

    virtual void send(const std::string& data,
                      const std::string& remote_ip,
                      uint16_t remote_port) = 0;

    virtual void stop() = 0;
};
#endif //SIPTRAININGPROJECT_IUDPTRANSPORT_H