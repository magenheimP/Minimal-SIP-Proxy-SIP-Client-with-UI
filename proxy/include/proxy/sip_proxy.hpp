//
// Created by lazarstani on 3/17/26.
//

#ifndef SIPTRAININGPROJECT_SIP_PROXY_HPP
#define SIPTRAININGPROJECT_SIP_PROXY_HPP
#pragma once

#include <memory>
#include <atomic>

#include "../../../networking/include/networking/udp_transport.hpp"
#include "../../../networking/include/networking/thread_pool.hpp"
#include "../../../networking/include/networking/dispatcher.hpp"

#include "../proxy/registration_table.hpp"
#include "../proxy/register_handler.hpp"

#include "../proxy/sip_router.hpp"
#include "../../../metrics/include/metrics_server.hpp"

namespace proxy {

    class SIPProxy {
    public:
        SIPProxy(size_t worker_threads = 4);

        void start(uint16_t port);
        void stop();

    private:
        void handle_packet(const common::RawPacket& pkt);

        void log_custom_headers(const common::SIPMessage& message);

        void log_modified_headers(const common::SIPMessage& message,
                                  const CallContext& context);

    private:
        std::atomic<bool> running_{false};

        ThreadPool thread_pool_;
        UdpTransport transport_;
        Dispatcher dispatcher_;

        RegistrationTable registration_table_;
        RegisterHandler register_handler_;
        SIPRouter router_;

        MetricsServer metrics_server_;
    };

}

#endif //SIPTRAININGPROJECT_SIP_PROXY_HPP