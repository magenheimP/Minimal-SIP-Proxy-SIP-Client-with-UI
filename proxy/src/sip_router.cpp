//
// Created by tamara on 3/12/26.
//

#include "../include/proxy/sip_router.hpp"
#include "../../common/include/common/logger.hpp"

namespace proxy {

SIPRouter::SIPRouter(RegistrationTable& table)
    : table_(table),
      handler_(table) {}

    RoutingResult SIPRouter::route(const common::SIPMessage& message, const std::string& sender_ip, uint16_t sender_port) {
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

    const std::string method = message.get_method();

    if (method == "REGISTER") {
        return handle_register(message);
    }

    if (method == "INVITE") {
        return handle_invite(message, sender_ip, sender_port);
    }

    if (method == "ACK") {
        return handle_ack(message);
    }

    if (method == "BYE") {
        return handle_bye(message);
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

RoutingResult SIPRouter::handle_register(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    if (!has_required_register_headers(message)) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "BAD_REQUEST",
            "REGISTER request is missing one or more required headers"
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);
        return result;
    }

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

RoutingResult SIPRouter::handle_invite(const common::SIPMessage& message,
                                        const std::string& sender_ip,
                                        uint16_t sender_port) {
    const std::string call_id = message.get_header("Call-ID");

    if (!has_required_invite_headers(message)) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "BAD_REQUEST",
            "INVITE request is missing one or more required headers"
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);

        return result;
    }

    const std::string from = message.get_header("From");
    const std::string to = message.get_header("To");

    const std::string caller = extract_sip_identity(from);
    const std::string callee = extract_sip_identity(to);

    if (caller.empty() || callee.empty()) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "BAD_REQUEST",
            "Failed to extract caller/callee SIP identity from From/To headers"
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
        "Looking up callee = " + callee);

    const auto callee_contact = table_.get_contact(callee);

    if (!callee_contact) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "NOT_FOUND",
            "Callee not registered: " + callee);

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 404 Not Found", message);
        result.user = callee;

        return result;
    }

    common::SIPMessage forwarded_message = message;
    forwarded_message.prepend_header("Via", build_proxy_via());

    auto session = registry_.create_call(call_id, caller, callee);
    if (!session) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "DUPLICATE_INVITE",
            "INVITE with existing Call-ID detected, rejecting request"
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);
        result.user = callee;

        return result;
    }

    store_call_context(message,
                   caller,
                   callee,
                   *callee_contact,
                   sender_ip,
                   sender_port);

    session->handle_event(CallEvent::INVITE);

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "FORWARD_REQUEST",
        "Forwarding INVITE to callee = " + callee + " contact = " + *callee_contact);

    RoutingResult result;
    result.action = RoutingAction::ForwardRequest;
    result.message = forwarded_message;
    result.contact = *callee_contact;
    result.user = callee;

    return result;
}

RoutingResult SIPRouter::handle_ack(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    const auto context = get_call_context(call_id);
    if (!context) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "CALL_NOT_FOUND",
            "ACK received for unknown Call-ID"
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);

        return result;
    }

    const std::string sender = extract_sip_identity(message.get_header("From"));

    std::string destination_contact;
    std::string destination_user;

    if (sender == context->caller) {
        destination_contact = context->callee_contact;
        destination_user = context->callee;
    } else {
        destination_contact.clear();
        destination_user = context->caller;
    }

    common::SIPMessage forwarded_message = message;
    forwarded_message.prepend_header("Via", build_proxy_via());

    auto session = registry_.find_call(call_id);
    if (session) {
        session->handle_event(CallEvent::ACK);
    }

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "FORWARD_REQUEST",
        "Forwarding ACK to user = " + destination_user + ", contact = " + destination_contact);

    RoutingResult result;
    result.action = RoutingAction::ForwardRequest;
    result.message = forwarded_message;
    result.user = destination_user;

    if (sender == context->caller) {
        result.contact = destination_contact;
    } else {
        result.ip = context->caller_ip;
        result.port = context->caller_port;
    }

    return result;
}

RoutingResult SIPRouter::handle_bye(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    const auto context = get_call_context(call_id);
    if (!context) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "CALL_NOT_FOUND",
            "BYE received for unknown Call-ID"
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);

        return result;
    }

    const std::string sender = extract_sip_identity(message.get_header("From"));

    std::string destination_contact;
    std::string destination_user;

    if (sender == context->caller) {
        destination_contact = context->callee_contact;
        destination_user = context->callee;
    } else {
        destination_contact.clear();
        destination_user = context->caller;
    }

    common::SIPMessage forwarded_message = message;
    forwarded_message.prepend_header("Via", build_proxy_via());

    auto session = registry_.find_call(call_id);
    if (session) {
        session->handle_event(CallEvent::BYE);
    }

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "FORWARD_REQUEST",
        "Forwarding BYE to user = " + destination_user + ", contact = " + destination_contact);

    RoutingResult result;
    result.action = RoutingAction::ForwardRequest;
    result.message = forwarded_message;
    result.user = destination_user;

    if (sender == context->caller) {
        result.contact = destination_contact;
    } else {
        result.ip = context->caller_ip;
        result.port = context->caller_port;
    }

    return result;
}

RoutingResult SIPRouter::handle_response(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    const auto context = get_call_context(call_id);
    if (!context) {
        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "CALL_NOT_FOUND",
            "Response received for unknown Call-ID"
        );

        return {};
    }

    common::SIPMessage forwarded_message = message;

    strip_proxy_via(forwarded_message);

    const int status_code = forwarded_message.get_status_code();
    const std::string cseq = forwarded_message.get_header("CSeq");

    auto session = registry_.find_call(call_id);
    if (session) {
        if (status_code == 100) {
            session->handle_event(CallEvent::TRYING_100);
        } else if (status_code == 180) {
            session->handle_event(CallEvent::RINGING_180);
        } else if (status_code == 200 && cseq.find("INVITE") != std::string::npos) {
            session->handle_event(CallEvent::OK_200);
        }
    }

    if (status_code == 200 && cseq.find("BYE") != std::string::npos) {
        registry_.remove_call(call_id);
        call_contexts_.erase(call_id);

        common::Logger::instance("proxy.log").log(
            "SIPRouter",
            call_id,
            "CALL_REMOVED",
            "Removed call context and session after 200 OK to BYE"
        );
    }

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "FORWARD_RESPONSE",
        "Forwarding SIP response with status = " +
        std::to_string(status_code) + " to caller = " +
        context->caller_ip + ":" +
        std::to_string(context->caller_port));

    RoutingResult result;
    result.action = RoutingAction::ForwardResponse;
    result.message = forwarded_message;
    result.ip = context->caller_ip;
    result.port = context->caller_port;
    result.user = context->caller;

    return result;
}

bool SIPRouter::has_required_register_headers(const common::SIPMessage& message) {
    return message.has_header("Via") &&
           message.has_header("From") &&
           message.has_header("To") &&
           message.has_header("Call-ID") &&
           message.has_header("CSeq") &&
           message.has_header("Contact") &&
           message.has_header("Content-Length");
}

bool SIPRouter::has_required_invite_headers(const common::SIPMessage& message) {
    return message.has_header("Via") &&
           message.has_header("From") &&
           message.has_header("To") &&
           message.has_header("Call-ID") &&
           message.has_header("CSeq") &&
           message.has_header("Content-Length");
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

std::string SIPRouter::build_proxy_via() {
    return "SIP/2.0/UDP proxy.local:5060";
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

void SIPRouter::store_call_context(const common::SIPMessage& message,
                                   const std::string& caller,
                                   const std::string& callee,
                                   const std::string& callee_contact,
                                   const std::string& caller_ip,
                                   uint16_t caller_port) {

    const std::string call_id = message.get_header("Call-ID");

    CallContext context;
    context.call_id = call_id;
    context.caller = caller;
    context.callee = callee;

    context.caller_ip = caller_ip;
    context.caller_port = caller_port;

    context.callee_contact = callee_contact;

    call_contexts_[call_id] = context;

    common::Logger::instance("proxy.log").log(
        "SIPRouter",
        call_id,
        "CONTEXT_STORED",
        "Stored call context caller = " + caller +
        " ip = " + caller_ip +
        ": " + std::to_string(caller_port));
}

std::optional<CallContext> SIPRouter::get_call_context(const std::string& call_id) const {
    auto it = call_contexts_.find(call_id);
    if (it == call_contexts_.end()) {
        return std::nullopt;
    }

    return it->second;
}

void SIPRouter::strip_proxy_via(common::SIPMessage& message) {
    const std::string proxy_via = build_proxy_via();

    for (auto it = message.headers.begin(); it != message.headers.end(); ++it) {
        if (it->name == "Via" && it->value == proxy_via) {
            message.headers.erase(it);
            return;
        }
    }
}

}