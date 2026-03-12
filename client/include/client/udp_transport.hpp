#pragma once
#include <string>
#include <functional>
#include <cstdint>


using PacketCallback = std::function<void(const std::string&,
                                          const std::string&,
                                          uint16_t)>;

class UdpTransport {
public:
    UdpTransport();
    ~UdpTransport();

    void start(uint16_t local_port, PacketCallback on_packet);

    void stop();

    void send(const std::string& data,
              const std::string& dest_ip,
              uint16_t           dest_port);

private:
    struct Impl;
    Impl* impl_;
};
