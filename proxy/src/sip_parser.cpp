//
// Created by tamara on 3/9/26.
//

#include "proxy/sip_parser.hpp"

#include <sstream>

namespace proxy {

common::SIPMessage SIPParser::parse(const std::string& raw_message) {
  common::SIPMessage message;

  std::istringstream stream(raw_message);
  std::string line;

  bool first_line = true;

  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (line.empty()) {
      break;
    }

    if (first_line) {
      parse_start_line(message, line);
      first_line = false;
      continue;
    }

    parse_header_line(message, line);
  }

  return message;
}

void SIPParser::parse_start_line(common::SIPMessage& message,
                                 const std::string& line) {
  if (line.empty()) {
    return;
  }

  message.start_line = line;
}

void SIPParser::parse_header_line(common::SIPMessage& message,
                                  const std::string& line) {
  const std::size_t separator_pos = line.find(':');

  if (separator_pos == std::string::npos) {
    return;
  }

  const std::string name = trim(line.substr(0, separator_pos));
  const std::string value = trim(line.substr(separator_pos + 1));

  if (name.empty()) {
    return;
  }

  message.add_header(name, value);
}

std::string SIPParser::trim(const std::string& value) {
  const std::size_t start = value.find_first_not_of(" ");

  if (start == std::string::npos) {
    return "";
  }

  const std::size_t end = value.find_last_not_of(" ");
  return value.substr(start, end - start + 1);
}

} // namespace proxy