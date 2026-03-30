//
// Created by lazarstani on 3/17/26.
//

#include "proxy/sip_proxy.hpp"
#include "common/logger.hpp"
#include "proxy/sip_parser.hpp"

#include <unordered_set>

namespace proxy {

    static const std::unordered_set<std::string> STANDARD_SIP_HEADERS = {
        "Via", "From", "To", "Call-ID", "CSeq",
        "Contact", "Content-Length", "Content-Type"
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

    void SIPProxy::start(uint16_t udp_port, uint16_t tcp_port)
    {
        running_ = true;

        common::Logger::instance("sip_proxy.log")
            .log("PROXY", "-", "START", "Proxy starting");

        dispatcher_.start();

        transport_.start(udp_port,
            [this](const std::string& data,
                   const std::string& ip,
                   uint16_t           port)
            {
                dispatcher_.push(common::RawPacket{data, ip, port});
            });

        common::Logger::instance().log(
            "NETWORK", "-", "INFO",
            "UDP server running on port " + std::to_string(udp_port));

        if (tcp_port > 0) {
            tcp_enabled_ = true;

            tcp_transport_.start(tcp_port,
                [this](const std::string& data,
                       const std::string& ip,
                       uint16_t           port)
                {
                    common::RawPacket pkt;
                    pkt.data      = data;
                    pkt.ip        = ip;
                    pkt.port      = port;
                    pkt.transport = common::TransportType::TCP;
                    dispatcher_.push(pkt);
                });

            common::Logger::instance().log(
                "NETWORK", "-", "INFO",
                "TCP server running on port " + std::to_string(tcp_port));
        }
    }

    void SIPProxy::stop()
    {
        if (!running_) return;

        running_ = false;

        common::Logger::instance().log(
            "PROXY", "-", "STOP", "Proxy shutting down");

        transport_.stop();

        if (tcp_enabled_)
            tcp_transport_.stop();

        dispatcher_.stop();
        thread_pool_.shutdown();
    }

    void SIPProxy::send_response(const std::string&    data,
                                  const std::string&    ip,
                                  uint16_t              port,
                                  common::TransportType transport)
    {
        if (tcp_enabled_ && transport == common::TransportType::TCP)
            tcp_transport_.send(data, ip, port);
        else
            transport_.send(data, ip, port);
    }

    void SIPProxy::log_custom_headers(const common::SIPMessage& message)
    {
        const std::string call_id = message.get_header("Call-ID");

        for (const auto& header : message.headers) {
            if (STANDARD_SIP_HEADERS.find(header.name) == STANDARD_SIP_HEADERS.end()) {
                common::Logger::instance().log(
                    "NETWORK", call_id, "CUSTOM_HEADER",
                    header.name + ": " + header.value);
            }
        }
    }

    void SIPProxy::log_modified_headers(const common::SIPMessage& message,
                                        const CallContext&         context)
    {
        const std::string call_id = message.get_header("Call-ID");
        const std::string sender  = message.get_header("From");

        const bool is_caller = sender.find(context.caller) != std::string::npos;
        const auto& stored   = is_caller
                               ? context.caller_stored_headers
                               : context.callee_stored_headers;

        if (!is_caller && stored.empty()) return;

        for (const auto& header : message.headers) {
            if (STANDARD_SIP_HEADERS.find(header.name) == STANDARD_SIP_HEADERS.end())
                continue;
            if (header.name == "Via")  continue;
            if (header.name == "CSeq") continue;

            const auto it = stored.find(header.name);
            if (it != stored.end() && it->second != header.value) {
                common::Logger::instance().log(
                    "NETWORK", call_id, "HEADER_MODIFIED",
                    header.name + ": " + header.value +
                    " (was: " + it->second + ")");
            }
        }
    }

    void SIPProxy::handle_packet(const common::RawPacket& pkt)
    {
        common::Logger::instance().log(
            "NETWORK", "-", "RECV",
            "Packet from " + pkt.ip + ":" + std::to_string(pkt.port));

        common::SIPMessage message = SIPParser::parse(pkt.data);

        common::Logger::instance().log(
            common::LogLevel::INFO, "NETWORK",
            message.get_header("Call-ID"), "SIP_MESSAGE_IN", message);

        log_custom_headers(message);

        const std::string call_id = message.get_header("Call-ID");
        const auto context = router_.get_call_context(call_id);
        if (context) {
            log_modified_headers(message, *context);
        }

        if (message.is_request() && message.get_method() == "INVITE") {
            const std::string caller           = message.get_header("From");
            const std::string contact_in_invite = message.get_header("Contact");

            const auto sip_pos = caller.find("sip:");
            const auto gt_pos  = caller.find('>', sip_pos);
            if (sip_pos != std::string::npos && !contact_in_invite.empty()) {
                const std::string caller_identity = caller.substr(sip_pos + 4,
                    gt_pos != std::string::npos
                        ? gt_pos - sip_pos - 4
                        : std::string::npos);

                const auto registered_contact =
                    router_.get_registered_contact(caller_identity);
                if (registered_contact && *registered_contact != contact_in_invite) {
                    common::Logger::instance().log(
                        "NETWORK", call_id, "HEADER_MODIFIED",
                        "Contact: " + contact_in_invite +
                        " (was: " + *registered_contact + ")");
                }
            }
        }


        RoutingResult result = router_.route(message, pkt.ip, pkt.port,
                                             pkt.transport);

        switch (result.action) {

        case RoutingAction::RespondLocally:
            send_response(
                result.response.serialize(),
                pkt.ip, pkt.port, pkt.transport);

            common::Logger::instance().log(
                "NETWORK", result.response.get_header("Call-ID"),
                "SEND", "Local response sent to " + pkt.ip);

            common::Logger::instance().log(
                common::LogLevel::INFO, "NETWORK",
                result.response.get_header("Call-ID"),
                "SIP_MESSAGE_OUT", result.response);

            common::Logger::instance().separator();
            break;

        case RoutingAction::ForwardRequest:
            if (result.contact) {
                std::string contact  = *result.contact;
                auto pos_at    = contact.find('@');
                auto pos_colon = contact.find(':', pos_at);
                std::string ip = contact.substr(pos_at + 1,
                                                pos_colon - pos_at - 1);
                uint16_t    port = std::stoi(contact.substr(pos_colon + 1));


                send_response(result.message.serialize(), ip, port,
                              result.callee_transport);

                common::Logger::instance().log(
                    "NETWORK", result.message.get_header("Call-ID"),
                    "FORWARD",
                    "Forwarded request to " + ip + ":" + std::to_string(port));

                common::Logger::instance().log(
                    common::LogLevel::INFO, "NETWORK",
                    result.message.get_header("Call-ID"),
                    "SIP_MESSAGE_OUT", result.message);

                common::Logger::instance().separator();
            } else {
                send_response(result.message.serialize(),
                              result.ip, result.port,
                              result.callee_transport);

                common::Logger::instance().log(
                    "NETWORK", result.message.get_header("Call-ID"),
                    "FORWARD",
                    "Forwarded request to " + result.ip +
                    ":" + std::to_string(result.port));

                common::Logger::instance().log(
                    common::LogLevel::INFO, "NETWORK",
                    result.message.get_header("Call-ID"),
                    "SIP_MESSAGE_OUT", result.message);

                common::Logger::instance().separator();
            }
            break;

        case RoutingAction::ForwardResponse:
            send_response(
                result.message.serialize(),
                result.ip, result.port, pkt.transport);

            common::Logger::instance().log(
                "NETWORK", result.message.get_header("Call-ID"),
                "FORWARD",
                "Forwarded response to " + result.ip +
                ":" + std::to_string(result.port));

            common::Logger::instance().log(
                common::LogLevel::INFO, "NETWORK",
                result.message.get_header("Call-ID"),
                "SIP_MESSAGE_OUT", result.message);

            common::Logger::instance().separator();
            break;

        case RoutingAction::Ignore:
            common::Logger::instance().log(
                "NETWORK", "-", "IGNORE", "Message ignored");
            break;
        }
    }

}
