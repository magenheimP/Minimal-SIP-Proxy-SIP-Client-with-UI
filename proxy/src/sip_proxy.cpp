//
// Created by lazarstani on 3/17/26.
//




#include "proxy/sip_proxy.hpp"

#include "common/logger.hpp"
#include "proxy/sip_parser.hpp"

namespace proxy {

    SIPProxy::SIPProxy(size_t worker_threads)
        : thread_pool_(worker_threads),
          dispatcher_(thread_pool_,
              [this](const common::RawPacket& pkt) {
                  handle_packet(pkt);
              }),
          register_handler_(registration_table_),
            router_(registration_table_)
    {
    }

    void SIPProxy::start(uint16_t port)
    {
        running_ = true;

        common::Logger::instance("sip_proxy.log")
            .log("PROXY", "-", "START", "Proxy starting");

        dispatcher_.start();

        transport_.start(port,
            [this](const std::string& data,
                   const std::string& ip,
                   uint16_t port)
            {
                dispatcher_.push(common::RawPacket{data, ip, port});
            });

        common::Logger::instance().log(
            "NETWORK", "-", "INFO",
            "UDP server running on port " + std::to_string(port));
    }

    void SIPProxy::stop()
    {
        if (!running_) return;

        running_ = false;

        common::Logger::instance().log(
            "PROXY", "-", "STOP", "Proxy shutting down");

        transport_.stop();
        dispatcher_.stop();
        thread_pool_.shutdown();
    }

    void SIPProxy::handle_packet(const common::RawPacket& pkt)
    {
        common::Logger::instance().log(
            "NETWORK",
            "-",
            "RECV",
            "Packet from " + pkt.ip + ":" + std::to_string(pkt.port));

        // Parse SIP message
        common::SIPMessage message = SIPParser::parse(pkt.data);

        // Route it
        RoutingResult result = router_.route(message, pkt.ip, pkt.port);

        switch (result.action) {

        case RoutingAction::RespondLocally:
            transport_.send(
                result.response.serialize(),
                pkt.ip,
                pkt.port);

            common::Logger::instance().log(
                "NETWORK",
                result.response.get_header("Call-ID"),
                "SEND",
                "Local response sent to " + pkt.ip);
            break;

        case RoutingAction::ForwardRequest:
            if (result.contact) {
                // contact = "user@ip:port"
                std::string contact = *result.contact;

                auto pos_at = contact.find('@');
                auto pos_colon = contact.find(':', pos_at);

                std::string ip = contact.substr(pos_at + 1, pos_colon - pos_at - 1);
                uint16_t port = std::stoi(contact.substr(pos_colon + 1));

                transport_.send(
                    result.message.serialize(),
                    ip,
                    port);

                common::Logger::instance().log(
                    "NETWORK",
                    result.message.get_header("Call-ID"),
                    "FORWARD",
                    "Forwarded request to " + ip + ":" + std::to_string(port));
            } else {
                transport_.send(
                    result.message.serialize(),
                    result.ip,
                    result.port);

                common::Logger::instance().log(
                    "NETWORK",
                    result.message.get_header("Call-ID"),
                    "FORWARD",
                    "Forwarded request to " + result.ip + ":" + std::to_string(result.port));
            }
            break;

        case RoutingAction::ForwardResponse:
            transport_.send(
                result.message.serialize(),
                result.ip,
                result.port);

            common::Logger::instance().log(
                "NETWORK",
                result.message.get_header("Call-ID"),
                "FORWARD",
                "Forwarded response to " + result.ip + ":" + std::to_string(result.port));
            break;

        case RoutingAction::Ignore:
            common::Logger::instance().log(
                "NETWORK",
                "-",
                "IGNORE",
                "Message ignored");
            break;
        }
    }

}