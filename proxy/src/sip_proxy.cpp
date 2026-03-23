//
// Created by lazarstani on 3/17/26.
//




#include "proxy/sip_proxy.hpp"

#include "common/logger.hpp"
#include "proxy/sip_parser.hpp"
#include <unordered_set>
#include <algorithm>
#include <cctype>

namespace proxy {

        static const std::unordered_set<std::string> STANDARD_SIP_HEADERS = {
        "via", "from", "to", "call-id", "cseq",
        "contact", "content-length", "content-type"
    };

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

        common::Logger::instance().log(
            common::LogLevel::INFO,
            "NETWORK",
            message.get_header("Call-ID"),
            "SIP_MESSAGE_IN",
            message);
        log_custom_headers(message);


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

            common::Logger::instance().log(
                common::LogLevel::INFO,
                "NETWORK",
                result.response.get_header("Call-ID"),
                "SIP_MESSAGE_OUT",
                result.response);

            common::Logger::instance().separator();
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

                common::Logger::instance().log(
                    common::LogLevel::INFO,
                    "NETWORK",
                    result.message.get_header("Call-ID"),
                    "SIP_MESSAGE_OUT",
                    result.message);

                common::Logger::instance().separator();
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

                common::Logger::instance().log(
                    common::LogLevel::INFO,
                    "NETWORK",
                    result.message.get_header("Call-ID"),
                    "SIP_MESSAGE_OUT",
                    result.message);

                common::Logger::instance().separator();
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

            common::Logger::instance().log(
                common::LogLevel::INFO,
                "NETWORK",
                result.message.get_header("Call-ID"),
                "SIP_MESSAGE_OUT",
                result.message);

            common::Logger::instance().separator();
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

    void SIPProxy::log_custom_headers(const common::SIPMessage& message)
    {
        const std::string call_id = message.get_header("Call-ID");

        for (const auto& header : message.headers) {
            // Normalize to lowercase for case-insensitive lookup
            std::string lower_name = header.name;
            std::transform(lower_name.begin(), lower_name.end(),
                           lower_name.begin(), ::tolower);

            if (!STANDARD_SIP_HEADERS.contains(lower_name)) {
                // Header name not in the standard set, custom header injected by the client
                common::Logger::instance().log(
                    "NETWORK",
                    call_id,
                    "CUSTOM_HEADER",
                    header.name + ": " + header.value);
            } else if (lower_name == "via" &&
                       header.value.find("proxy.local") == std::string::npos) {
                // Via header present but not added by this proxy, it was set
                // or replaced by the client directly
                common::Logger::instance().log(
                    "NETWORK",
                    call_id,
                    "HEADER_MODIFIED",
                    header.name + ": " + header.value);
                       }
        }
    }

}