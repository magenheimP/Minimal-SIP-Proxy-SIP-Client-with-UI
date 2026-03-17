
//
// Created by tamara on 3/9/26.
//

#pragma once

#include <string>

#include "../../common/include/common/sip_message.hpp"

namespace proxy {

class SIPParser {
public:
    static common::SIPMessage parse(const std::string& raw_message);

private:
    static void parse_start_line(common::SIPMessage& message,
                                const std::string& line);

    static void parse_header_line(common::SIPMessage& message,
                                    const std::string& line);

    static void parse_body(common::SIPMessage& message,
                         const std::string& raw_body);

    static int get_content_length(const common::SIPMessage& message);

    static std::string trim(const std::string& value);
};

} // namespace proxy