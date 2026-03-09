#pragma once

#include <string>
#include <vector>

namespace common {

    struct SIPHeader {
        std::string name;
        std::string value;
    };

    struct SIPMessage {
        std::string start_line;
        std::vector<SIPHeader> headers;
        std::string body;

        std::string get_header(const std::string& name) const;
        void set_header(const std::string& name, const std::string& value);
        void add_header(const std::string& name, const std::string& value);

        std::string serialize() const;
    };

}