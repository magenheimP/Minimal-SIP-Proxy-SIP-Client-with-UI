//
// Created by tamara on 3/9/26.
//

#include "../include/proxy/register_handler.hpp"
#include "../include/proxy/registration_table.hpp"
#include <iostream>

namespace proxy {

static common::SIPMessage make_response(
    const std::string& status,
    const common::SIPMessage& request) {

  common::SIPMessage response;

  response.start_line = status;

  response.add_header("Via", request.get_header("Via"));
  response.add_header("From", request.get_header("From"));
  response.add_header("To", request.get_header("To"));
  response.add_header("Call-ID", request.get_header("Call-ID"));
  response.add_header("CSeq", request.get_header("CSeq"));
  response.add_header("Content-Length", "0");

  return response;
}

RegisterHandler::RegisterHandler(RegistrationTable& table)
    : table_(table) {}

common::SIPMessage RegisterHandler::handle(const common::SIPMessage& message) const {
  // For initial demo we assume that SIP messages are valid.
  // Full header validation will be implemented in next update.

  if (message.start_line.rfind("REGISTER", 0) != 0) {
    return make_response("SIP/2.0 400 Bad Request", message);
  }

  const std::string from = message.get_header("From");
  const std::string contact = message.get_header("Contact");

  if (from.empty() || contact.empty()) {
    return make_response("SIP/2.0 400 Bad Request", message);
  }

  table_.register_user(from, contact);

  return make_response("SIP/2.0 200 OK", message);
}

}
