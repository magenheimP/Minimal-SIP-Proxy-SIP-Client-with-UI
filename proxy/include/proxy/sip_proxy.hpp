#ifndef SIPTRAININGPROJECT_SIP_PROXY_HPP
#define SIPTRAININGPROJECT_SIP_PROXY_HPP
#pragma once

#include <memory>
#include <atomic>

#include "../../../networking/include/networking/udp_transport.hpp"
#include "../../../networking/include/networking/thread_pool.hpp"
#include "../../../networking/include/networking/dispatcher.hpp"

#include "../../../networking/include/networking/tcp_transport.hpp"

#include "../proxy/registration_table.hpp"
#include "../proxy/register_handler.hpp"
#include "../proxy/sip_router.hpp"

namespace proxy {

    class SIPProxy {
    public:
        SIPProxy(size_t worker_threads = 4);

        void start(uint16_t udp_port, uint16_t tcp_port = 0);
        void stop();

    private:
        void handle_packet(const common::RawPacket& pkt);

        void log_custom_headers(const common::SIPMessage& message);

        void log_modified_headers(const common::SIPMessage& message,
                                  const CallContext& context);


        void send_response(const std::string&    data,
                           const std::string&    ip,
                           uint16_t              port,
                           common::TransportType transport);

    private:
        std::atomic<bool> running_{false};

        ThreadPool   thread_pool_;
        UdpTransport transport_;
        Dispatcher   dispatcher_;

        TcpTransport tcp_transport_;
        bool         tcp_enabled_{false};

        RegistrationTable registration_table_;
        RegisterHandler   register_handler_;
        SIPRouter         router_;
    };

}

#endif //SIPTRAININGPROJECT_SIP_PROXY_HPP
