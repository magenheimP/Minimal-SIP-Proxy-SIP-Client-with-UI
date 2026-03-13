//
// Created by tamara on 3/12/26.
//

#include "../include/proxy/sip_router.hpp"
#include "../../common/include/common/logger.hpp"

namespace proxy {

SIPRouter::SIPRouter(RegistrationTable& table)
    : table_(table),
      handler_(table) {}

RoutingResult SIPRouter::route(const common::SIPMessage& message) const {
    const std::string call_id = message.get_header("Call-ID");

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "RECEIVED",
        "Routing SIP message: " + message.start_line
    );

    if (message.is_response()) {
        return handle_response(message);
    }

    if (!message.is_request()) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "IGNORED",
            "Message is neither a valid request nor response"
        );

        return {};
    }

    const std::string method = extract_method(message.start_line);

    if (method == "REGISTER") {
        return handle_register(message);
    }

    if (method == "INVITE") {
        return handle_invite(message);
    }

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "NOT_IMPLEMENTED",
        "Unsupported SIP method: " + method
    );

    RoutingResult result;
    result.action = RoutingAction::RespondLocally;
    result.response = make_error_response("SIP/2.0 501 Not Implemented", message);

    return result;
}

RoutingResult SIPRouter::handle_register(const common::SIPMessage& message) const {
    const std::string call_id = message.get_header("Call-ID");

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "REGISTER_DISPATCH",
        "Dispatching REGISTER to RegisterHandler"
    );

    RoutingResult result;
    result.action = RoutingAction::RespondLocally;
    result.response = handler_.handle(message);

    return result;
}

RoutingResult SIPRouter::handle_invite(const common::SIPMessage& message) const {
    const std::string call_id = message.get_header("Call-ID");

    const std::string to = message.get_header("To");
    if (to.empty()) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "BAD_REQUEST",
            "INVITE missing To header"
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);

        return result;
    }

    const std::string user = extract_sip_identity(to);
    if (user.empty()) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "BAD_REQUEST",
            "Failed to extract SIP identity from To header: " + to
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);

        return result;
    }

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "INVITE_LOOKUP",
        "Looking up user=" + user
    );

    const auto contact = table_.get_contact(user);

    if (!contact) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "NOT_FOUND",
            "User not registered: " + user
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 404 Not Found", message);
        result.user = user;

        return result;
    }

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "FORWARD_REQUEST",
        "Forwarding INVITE to user=" + user + " contact=" + *contact
    );

    RoutingResult result;
    result.action = RoutingAction::ForwardRequest;
    result.message = message;
    result.contact = *contact;
    result.user = user;

    return result;
}

RoutingResult SIPRouter::handle_response(const common::SIPMessage& message) const {
    const std::string call_id = message.get_header("Call-ID");

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "FORWARD_RESPONSE",
        "Received SIP response"
    );

    RoutingResult result;
    result.action = RoutingAction::ForwardResponse;
    result.message = message;

    return result;
}

std::string SIPRouter::extract_method(const std::string& start_line) {
    const std::size_t space = start_line.find(' ');

    if (space == std::string::npos) {
        return start_line;
    }

    return start_line.substr(0, space);
}

std::string SIPRouter::extract_sip_identity(const std::string& header) {
    const std::size_t sip_pos = header.find("sip:");

    if (sip_pos == std::string::npos) {
        return "";
    }

    const std::size_t start = sip_pos + 4;
    std::size_t end = header.find('>', start);

    if (end == std::string::npos) {
        end = header.size();
    }

    return header.substr(start, end - start);
}

common::SIPMessage SIPRouter::make_error_response(
    const std::string& status,
    const common::SIPMessage& request) {

    common::SIPMessage response;

    response.start_line = status;

    const std::string via = request.get_header("Via");
    const std::string from = request.get_header("From");
    const std::string to = request.get_header("To");
    const std::string call_id = request.get_header("Call-ID");
    const std::string cseq = request.get_header("CSeq");

    if (!via.empty()) response.add_header("Via", via);
    if (!from.empty()) response.add_header("From", from);
    if (!to.empty()) response.add_header("To", to);
    if (!call_id.empty()) response.add_header("Call-ID", call_id);
    if (!cseq.empty()) response.add_header("CSeq", cseq);

    response.add_header("Content-Length", "0");

    return response;
}

}