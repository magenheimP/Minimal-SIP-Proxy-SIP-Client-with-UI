//
// Created by tamara on 3/10/26.
//

#include <cassert>
#include <iostream>

#include "proxy/register_handler.hpp"
#include "proxy/registration_table.hpp"
#include "proxy/sip_parser.hpp"
#include "common/sip_message.hpp"

int main() {

  proxy::RegistrationTable table;
  proxy::RegisterHandler handler(table);
  proxy::SIPParser parser;

  std::string raw =
      "REGISTER sip:server SIP/2.0\r\n"
      "Via: SIP/2.0/UDP client\r\n"
      "From: alice\r\n"
      "To: alice\r\n"
      "Call-ID: 12345\r\n"
      "CSeq: 1 REGISTER\r\n"
      "Contact: sip:alice@127.0.0.1\r\n"
      "Content-Length: 0\r\n"
      "\r\n";

  // Parse message
  common::SIPMessage message = parser.parse(raw);

  // First REGISTER
  common::SIPMessage response1 = handler.handle(message);
  assert(response1.start_line == "SIP/2.0 200 OK");

  std::string contact1 = table.get_contact("alice");
  assert(contact1 == "sip:alice@127.0.0.1");

  // Duplicate REGISTER
  common::SIPMessage response2 = handler.handle(message);
  assert(response2.start_line == "SIP/2.0 200 OK");

  std::string contact2 = table.get_contact("alice");
  assert(contact2 == "sip:alice@127.0.0.1");

  std::cout << "REGISTER tests passed\n";

  return 0;
}
