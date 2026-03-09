//
// Created by tamara on 3/6/26.
//

#include <cassert>
#include <string>

#include "proxy/sip_parser.hpp"

int main() {
  const std::string raw_message =
      "REGISTER sip:localhost SIP/2.0\r\n"
      "Via: SIP/2.0/UDP 127.0.0.1:5060\r\n"
      "From: <sip:alice@localhost>\r\n"
      "To: <sip:alice@localhost>\r\n"
      "Call-ID: reg123\r\n"
      "CSeq: 1 REGISTER\r\n"
      "Contact: <sip:alice@192.168.1.10>\r\n"
      "Content-Length: 0\r\n"
      "\r\n";

  const common::SIPMessage message = proxy::SIPParser::parse(raw_message);

  assert(message.start_line == "REGISTER sip:localhost SIP/2.0");
  assert(message.get_header("Via") == "SIP/2.0/UDP 127.0.0.1:5060");
  assert(message.get_header("From") == "<sip:alice@localhost>");
  assert(message.get_header("To") == "<sip:alice@localhost>");
  assert(message.get_header("Call-ID") == "reg123");
  assert(message.get_header("CSeq") == "1 REGISTER");
  assert(message.get_header("Contact") == "<sip:alice@192.168.1.10>");
  assert(message.get_header("Content-Length") == "0");

  return 0;
}