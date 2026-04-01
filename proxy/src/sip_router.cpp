//
// Created by tamara on 3/12/26.
//

#include "../include/proxy/sip_router.hpp"
#include "../../common/include/common/logger.hpp"
#include <unordered_set>

namespace proxy {

SIPRouter::SIPRouter(RegistrationTable& table)
    : table_(table),
      handler_(table) {}

RoutingResult SIPRouter::route(const common::SIPMessage& message,
                                const std::string& sender_ip,
                                uint16_t sender_port,
                                common::TransportType transport) {
    const std::string call_id = message.get_header("Call-ID");

    common::Logger::instance().log(
        common::LogLevel::INFO, "SIPRouter", call_id, "RECEIVED", message);

    if (message.is_response()) {
        return handle_response(message);
    }

    if (!message.is_request()) {
        common::Logger::instance().log(
            "SIPRouter",
            call_id,
            "IGNORED",
            "Message is neither a valid request nor response"
        );

        return {};
    }

    const std::string method = message.get_method();

    if (method == "REGISTER") {
        return handle_register(message, transport);
    }

    if (method == "INVITE") {
        return handle_invite(message, sender_ip, sender_port, transport);
    }

    if (method == "ACK") {
        return handle_ack(message);
    }

    if (method == "BYE") {
        return handle_bye(message);
    }

    common::Logger::instance().log(
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

RoutingResult SIPRouter::handle_register(const common::SIPMessage& message,
                                          common::TransportType transport) {
    const std::string call_id = message.get_header("Call-ID");

    if (!has_required_register_headers(message)) {
        common::Logger::instance().log(
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

    common::Logger::instance().log(
        "SIPRouter",
        call_id,
        "REGISTER_DISPATCH",
        "Dispatching REGISTER to RegisterHandler"
    );

    RoutingResult result;
    result.action = RoutingAction::RespondLocally;
    result.response = handler_.handle(message, transport);
    return result;
}

RoutingResult SIPRouter::handle_invite(const common::SIPMessage& message,
                                        const std::string& sender_ip,
                                        uint16_t sender_port,
                                        common::TransportType transport) {
    const std::string call_id = message.get_header("Call-ID");

    if (!has_required_invite_headers(message)) {
        common::Logger::instance().log(
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

    const std::string caller = extract_sip_identity(message.get_header("From"));
    const std::string callee = extract_sip_identity(message.get_header("To"));

    if (caller.empty() || callee.empty()) {
        common::Logger::instance().log(
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

    if (caller == callee) {
        common::Logger::instance().log(
            "SIPRouter",
            call_id,
            "INVALID_CALL",
            "Caller cannot call itself: " + caller
        );

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Bad Request", message);
        result.user = caller;
        return result;
    }

    common::Logger::instance().log(
        "SIPRouter",
        call_id,
        "INVITE_LOOKUP",
        "Looking up callee = " + callee);

    const auto callee_entry = table_.get_contact_entry(callee);

    if (!callee_entry) {
        common::Logger::instance().log(
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
    if (callee_entry->transport != transport) {
        common::Logger::instance().log(
            "SIPRouter",
            call_id,
            "WRONG PROTOCOL",
            "Callee is using different protocol " + callee);

        RoutingResult result;
        result.action = RoutingAction::RespondLocally;
        result.response = make_error_response("SIP/2.0 400 Different protocol", message);
        result.user = callee;

        return result;
    }

    common::SIPMessage forwarded_message = message;
    const uint16_t callee_port = (callee_entry->transport == common::TransportType::TCP)
                                 ? proxy_tcp_port_ : proxy_udp_port_;
    forwarded_message.prepend_header("Via", build_proxy_via(callee_entry->transport, callee_port));

    auto session = registry_.create_call(call_id, caller, callee);
    if (!session) {
        common::Logger::instance().log(
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
                   callee_entry->contact,
                   sender_ip,
                   sender_port,
                   transport,
                   callee_entry->transport);

    session->on_request(message);

    common::Logger::instance().log(
        "SIPRouter",
        call_id,
        "FORWARD_REQUEST",
        "Forwarding INVITE to callee = " + callee + " contact = " + callee_entry->contact);

    RoutingResult result;
    result.action = RoutingAction::ForwardRequest;
    result.message = forwarded_message;
    result.contact = callee_entry->contact;
    result.user = callee;
    result.callee_transport = callee_entry->transport;

    return result;
}

RoutingResult SIPRouter::handle_ack(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    const auto context = get_call_context(call_id);
    if (!context) {
        common::Logger::instance().log(
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
    const auto ack_transport = (sender == context->caller)
                               ? context->callee_transport
                               : context->caller_transport;
    const uint16_t ack_port = (ack_transport == common::TransportType::TCP)
                              ? proxy_tcp_port_ : proxy_udp_port_;
    forwarded_message.prepend_header("Via", build_proxy_via(ack_transport, ack_port));

    auto session = registry_.find_call(call_id);
    if (session) {
        session->on_request(message);
    }

    common::Logger::instance().log(
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
        result.callee_transport = context->callee_transport;
    } else {
        result.ip = context->caller_ip;
        result.port = context->caller_port;
        result.callee_transport = context->caller_transport;
    }

    return result;
}

RoutingResult SIPRouter::handle_bye(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    const auto context = get_call_context(call_id);
    if (!context) {
        common::Logger::instance().log(
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
    const auto bye_transport = (sender == context->caller)
                               ? context->callee_transport
                               : context->caller_transport;
    const uint16_t bye_port = (bye_transport == common::TransportType::TCP)
                              ? proxy_tcp_port_ : proxy_udp_port_;
    forwarded_message.prepend_header("Via", build_proxy_via(bye_transport, bye_port));

    auto session = registry_.find_call(call_id);
    if (session) {
        session->on_request(message);
    }

    common::Logger::instance().log(
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
        result.callee_transport = context->callee_transport;
    } else {
        result.ip = context->caller_ip;
        result.port = context->caller_port;
        result.callee_transport = context->caller_transport;
    }

    return result;
}

RoutingResult SIPRouter::handle_response(const common::SIPMessage& message) {
    const std::string call_id = message.get_header("Call-ID");

    const auto context = get_call_context(call_id);
    if (!context) {
        common::Logger::instance().log(
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
        session->on_response(forwarded_message);
    }

    if (status_code == 200 && cseq.find("INVITE") != std::string::npos) {
        std::lock_guard<std::mutex> lock(call_contexts_mutex_);
        auto it = call_contexts_.find(call_id);
        if (it != call_contexts_.end()) {
            for (const auto& header : forwarded_message.headers) {
                it->second.callee_stored_headers[header.name] = header.value;
            }
        }
    }

    if (status_code == 200 && cseq.find("BYE") != std::string::npos) {
        registry_.remove_call(call_id);

        // Acquire lock before erasing from the shared call_contexts_ map
        {
            std::lock_guard<std::mutex> lock(call_contexts_mutex_);
            call_contexts_.erase(call_id);
        }

        common::Logger::instance().log(
            "SIPRouter",
            call_id,
            "CALL_REMOVED",
            "Removed call context and session after 200 OK to BYE"
        );
    }

    const std::string top_via = forwarded_message.get_header("Via");
    const std::string caller_via_token = context->caller_ip + ":" + std::to_string(context->caller_port);
    const bool response_from_caller = !context->caller_ip.empty() &&
                                      top_via.find(caller_via_token) != std::string::npos;

    std::string dest_ip;
    uint16_t dest_port;
    std::string dest_user;

    if (response_from_caller && !context->callee_ip.empty()) {
        // Response came from the caller -> forward it to the callee
        dest_ip   = context->callee_ip;
        dest_port = context->callee_port;
        dest_user = context->callee;
    } else {
        // Response came from the callee -> forward it to the caller
        dest_ip   = context->caller_ip;
        dest_port = context->caller_port;
        dest_user = context->caller;
    }

    common::Logger::instance().log(
        "SIPRouter",
        call_id,
        "FORWARD_RESPONSE",
        "Forwarding SIP response with status = " +
        std::to_string(status_code) + " to " + dest_user +
        " = " + dest_ip + ":" + std::to_string(dest_port));

    static const std::unordered_set<std::string> STANDARD_HEADERS = {
        "Via", "From", "To", "Call-ID", "CSeq",
        "Contact", "Content-Length", "Content-Type"
    };

    const auto& stored = response_from_caller
                         ? context->callee_stored_headers
                         : context->caller_stored_headers;

    for (const auto& [name, value] : stored) {
        if (STANDARD_HEADERS.count(name) == 0 &&
            !forwarded_message.has_header(name)) {
            forwarded_message.add_header(name, value);
        }
    }

    RoutingResult result;
    result.action = RoutingAction::ForwardResponse;
    result.message = forwarded_message;
    result.ip   = dest_ip;
    result.port = dest_port;
    result.user = dest_user;

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

std::string SIPRouter::build_proxy_via(common::TransportType transport, uint16_t port) {
    const std::string protocol = (transport == common::TransportType::TCP)
                              ? "TCP" : "UDP";
    return "SIP/2.0/" + protocol + " proxy.local:" + std::to_string(port);
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
                                   uint16_t caller_port,
                                   common::TransportType caller_transport,
                                   common::TransportType callee_transport) {

    const std::string call_id = message.get_header("Call-ID");

    CallContext context;
    context.caller = caller;
    context.callee = callee;

    context.caller_ip = caller_ip;
    context.caller_port = caller_port;

    context.callee_contact = callee_contact;

    context.caller_transport = caller_transport;
    context.callee_transport = callee_transport;

    const auto pos_at = callee_contact.find('@');
    const auto pos_colon = callee_contact.find(':', pos_at);
    if (pos_at != std::string::npos && pos_colon != std::string::npos) {
        context.callee_ip = callee_contact.substr(pos_at + 1, pos_colon - pos_at - 1);
        context.callee_port = static_cast<uint16_t>(std::stoi(callee_contact.substr(pos_colon + 1)));
    }

    for (const auto& header : message.headers) {
        context.caller_stored_headers[header.name] = header.value;
    }

    const auto callee_registered_contact = table_.get_contact(callee);
    if (callee_registered_contact) {
        context.callee_stored_headers["Contact"] = *callee_registered_contact;
    }

    // Acquire lock before writing to the shared call_contexts_ map
    std::lock_guard<std::mutex> lock(call_contexts_mutex_);
    call_contexts_[call_id] = context;

    common::Logger::instance().log(
        "SIPRouter",
        call_id,
        "CONTEXT_STORED",
        "Stored call context caller = " + caller +
        " ip = " + caller_ip +
        ": " + std::to_string(caller_port));
}

std::optional<CallContext> SIPRouter::get_call_context(const std::string& call_id) const {
    // Acquire lock before reading from the shared call_contexts_ map
    std::lock_guard<std::mutex> lock(call_contexts_mutex_);
    auto it = call_contexts_.find(call_id);
    if (it == call_contexts_.end()) {
        return std::nullopt;
    }

    return it->second;
}

void SIPRouter::strip_proxy_via(common::SIPMessage& message) {
    const std::string proxy_host = "proxy.local";

    for (auto it = message.headers.begin(); it != message.headers.end(); ++it) {
        if (it->name == "Via" && it->value.find(proxy_host) != std::string::npos) {
            message.headers.erase(it);
            return;
        }
    }
}

}