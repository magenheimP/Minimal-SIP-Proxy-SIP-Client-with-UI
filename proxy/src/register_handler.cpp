//
// Created by tamara on 3/9/26.
//

#include "../include/proxy/register_handler.hpp"
#include "../include/proxy/registration_table.hpp"
#include "../../common/include/common/logger.hpp"

#include <iostream>

namespace proxy {
static std::string extract_sip_identity(const std::string& header_value) {
    const std::size_t sip_pos = header_value.find("sip:");
    if (sip_pos == std::string::npos) {
        return "";
    }

    const std::size_t start_pos = sip_pos + 4; //  skip "sip:"
    std::size_t end_pos = header_value.find('>', start_pos);
    if (end_pos == std::string::npos) {
        end_pos = header_value.size();
    }

    return header_value.substr(start_pos, end_pos - start_pos);
}

static common::SIPMessage make_response(
    const std::string& status,
    const common::SIPMessage& request) {

    common::SIPMessage response;

    response.start_line = status;

    const std::string via = request.get_header("Via");
    const std::string from = request.get_header("From");
    const std::string to = request.get_header("To");
    const std::string call_id = request.get_header("Call-ID");
    const std::string cseq = request.get_header("CSeq");

    if (!via.empty()) {
        response.add_header("Via", via);
    }

    if (!from.empty()) {
        response.add_header("From", from);
    }

    if (!to.empty()) {
        response.add_header("To", to);
    }

    if (!call_id.empty()) {
        response.add_header("Call-ID", call_id);
    }

    if (!cseq.empty()) {
        response.add_header("CSeq", cseq);
    }

    response.add_header("Content-Length", "0");

    return response;
}

RegisterHandler::RegisterHandler(RegistrationTable& table)
    : table_(table) {}

common::SIPMessage RegisterHandler::handle(const common::SIPMessage& message) const {

    const std::string call_id = message.get_header("Call-ID");

    common::Logger::instance("proxy.log").log(
        "RegisterHandler",
        call_id,
        "RECEIVED",
        "Received REGISTER handling request");

    if (message.start_line.rfind("REGISTER", 0) != 0) {
        common::Logger::instance("proxy.log").log(
            "RegisterHandler",
            call_id,
            "BAD_REQUEST",
            "Start line does not begin with REGISTER");

        return make_response("SIP/2.0 400 Bad Request", message);
    }

    const std::string from = message.get_header("From");
    const std::string contact = message.get_header("Contact");

    if (from.empty() || contact.empty()) {
        common::Logger::instance("proxy.log").log(
            "RegisterHandler",
            call_id,
            "BAD_REQUEST",
            "Missing From or Contact header");

        return make_response("SIP/2.0 400 Bad Request", message);
    }

    const std::string user_identity = extract_sip_identity(from);
    if (user_identity.empty()) {
        common::Logger::instance("proxy.log").log(
            "RegisterHandler",
            call_id,
            "BAD_REQUEST",
            "Failed to extract SIP identity from From header: " + from);

        return make_response("SIP/2.0 400 Bad Request", message);
    }

    table_.register_user(user_identity, contact);

    common::Logger::instance("proxy.log").log(
      "RegisterHandler",
      call_id,
      "REGISTERED",
      "User registered successfully: user = " + user_identity +
          ", contact = " + contact);

    return make_response("SIP/2.0 200 OK", message);
}

}
