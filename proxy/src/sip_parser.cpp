//
// Created by tamara on 3/9/26.
//

#include "../include/proxy/sip_parser.hpp"

#include <sstream>
#include <algorithm>

namespace proxy {

common::SIPMessage SIPParser::parse(const std::string& raw_message) {
    common::SIPMessage message;

    std::size_t separator_pos = raw_message.find("\r\n\r\n");
    std::size_t separator_length = 4;

    if (separator_pos == std::string::npos) {
        separator_pos = raw_message.find("\n\n");
        separator_length = 2;
    }

    std::string header_part;
    std::string body_part;

    if (separator_pos == std::string::npos) {
        header_part = raw_message;
    } else {
        header_part = raw_message.substr(0, separator_pos);
        body_part = raw_message.substr(separator_pos + separator_length);
    }


    std::istringstream stream(header_part);
    std::string line;

    bool first_line = true;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        if (first_line) {
            parse_start_line(message, line);
            first_line = false;
            continue;
        }

        parse_header_line(message, line);
    }

    parse_body(message, body_part);

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

void SIPParser::parse_body(common::SIPMessage& message,
                           const std::string& raw_body) {
    if (raw_body.empty()) {
        message.body.clear();
        return;
    }

    const int content_length = get_content_length(message);

    if (content_length < 0) {
        message.body = raw_body;
        return;
    }

    const std::size_t safe_length =
        std::min(raw_body.size(), static_cast<std::size_t>(content_length));

    message.body = raw_body.substr(0, safe_length);
}

int SIPParser::get_content_length(const common::SIPMessage& message) {
    const std::string value = message.get_header("Content-Length");

    if (value.empty()) {
        return -1;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return -1;
    }
}


std::string SIPParser::trim(const std::string& value) {
    const std::size_t start = value.find_first_not_of(" \t");

    if (start == std::string::npos) {
        return "";
    }

    const std::size_t end = value.find_last_not_of(" \t");
    return value.substr(start, end - start + 1);
}

}