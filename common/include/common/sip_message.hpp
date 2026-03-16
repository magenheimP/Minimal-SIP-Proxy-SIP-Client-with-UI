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
        bool has_header(const std::string& name) const;
        std::vector<std::string> get_headers(const std::string& name) const;
        void prepend_header(const std::string& name, const std::string& value);
        bool remove_first_header(const std::string& name);
        bool is_request() const;
        bool is_response() const;
        std::string get_method() const;
        int get_status_code() const;
        void update_content_length();

        std::string serialize() const;

    };

}