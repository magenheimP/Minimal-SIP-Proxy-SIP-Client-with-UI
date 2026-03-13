//
// Created by tamara on 3/12/26.
//


// NOTE:
// This router currently implements the minimal routing logic required for
// REGISTER handling and basic INVITE routing using the RegistrationTable.
//
// Future improvements planned for this module:
// - integration with CallSession / CallStateMachine
// - proper response routing based on Via headers
// - support for additional SIP methods (ACK, BYE, CANCEL)
// - insertion and removal of proxy Via headers for full SIP proxy behavior

#pragma once

#include <optional>
#include <string>

#include "../../common/include/common/sip_message.hpp"
#include "../include/proxy/register_handler.hpp"
#include "../include/proxy/registration_table.hpp"

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

        std::string user;
    };

    class SIPRouter {
    public:
        explicit SIPRouter(RegistrationTable& table);

        RoutingResult route(const common::SIPMessage& message) const;

    private:
        RoutingResult handle_register(const common::SIPMessage& message) const;
        RoutingResult handle_invite(const common::SIPMessage& message) const;
        RoutingResult handle_response(const common::SIPMessage& message) const;

        static std::string extract_method(const std::string& start_line);
        static std::string extract_sip_identity(const std::string& header);

        static common::SIPMessage make_error_response(
            const std::string& status,
            const common::SIPMessage& request);

    private:
        RegistrationTable& table_;
        RegisterHandler handler_;
    };

}