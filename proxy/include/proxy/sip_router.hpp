//
// Created by tamara on 3/12/26.
//

#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "../../common/include/common/sip_message.hpp"
#include "../include/proxy/register_handler.hpp"
#include "../include/proxy/registration_table.hpp"
#include "../include/proxy/call_registry.hpp"

namespace proxy {

    enum class RoutingAction {
        RespondLocally,
        ForwardRequest,
        ForwardResponse,
        Ignore
      };

    struct RoutingResult {
        RoutingAction action = RoutingAction::Ignore;

        common::SIPMessage response;

        common::SIPMessage message;

        std::optional<std::string> contact;

        std::string ip;
        uint16_t port = 0;

        std::string user;
    };

    struct CallContext {
        std::string caller;
        std::string callee;
        std::string caller_ip;
        uint16_t caller_port = 0;
        std::string callee_contact;
        std::string callee_ip;
        uint16_t callee_port = 0;
    };

    class SIPRouter {
    public:
        explicit SIPRouter(RegistrationTable& table);

        RoutingResult route(const common::SIPMessage& message,
                            const std::string& sender_ip,
                            uint16_t sender_port);

    private:
        RoutingResult handle_register(const common::SIPMessage& message);
        RoutingResult handle_invite(const common::SIPMessage& message, const std::string& sender_ip, uint16_t sender_port);
        RoutingResult handle_ack(const common::SIPMessage& message);
        RoutingResult handle_bye(const common::SIPMessage& message);
        RoutingResult handle_response(const common::SIPMessage& message);

        static bool has_required_register_headers(const common::SIPMessage& message);
        static bool has_required_invite_headers(const common::SIPMessage& message);

        static std::string extract_sip_identity(const std::string& header);

        static std::string build_proxy_via();

        static common::SIPMessage make_error_response(
            const std::string& status,
            const common::SIPMessage& request);

        void store_call_context(const common::SIPMessage& message,
                                const std::string& caller,
                                const std::string& callee,
                                const std::string& callee_contact,
                                const std::string& caller_ip,
                                uint16_t caller_port);

        std::optional<CallContext> get_call_context(const std::string& call_id) const;

        static void strip_proxy_via(common::SIPMessage& message);

    private:
        RegistrationTable& table_;
        RegisterHandler handler_;

        CallRegistry registry_;

        std::unordered_map<std::string, CallContext> call_contexts_;
    };

}