#include "common/sip_message.hpp"
#include <sstream>

namespace common {
    std::string SIPMessage::get_header(const std::string& name) const {
        for (const auto& h : headers) {
            if (h.name == name) {
                return h.value;
            }
        }
        return "";
    }

    void SIPMessage::add_header(const std::string& name,
                                const std::string& value) {
        headers.push_back({name, value});
    }

    bool SIPMessage::has_header(const std::string &name) const {
        for (const auto& h : headers) {
            if (h.name == name) {
                return true;
            }
        }
        return false;
    }

    bool SIPMessage::is_request() const {
        return !start_line.empty() && !is_response();
    }

    bool SIPMessage::is_response() const {
        return start_line.rfind("SIP/2.0", 0) == 0;
    }

    void SIPMessage::set_header(const std::string& name,
                                const std::string& value) {
        for (auto& h : headers) {
            if (h.name == name) {
                h.value = value;
                return;
            }
        }

        add_header(name, value);
    }

    std::string SIPMessage::serialize() const {
        std::ostringstream ss;

        // Start line
        ss << start_line << "\r\n";

        // Headers
        for (const auto& header : headers) {
            ss << header.name << ": " << header.value << "\r\n";
        }

        // Empty line separating headers and body
        ss << "\r\n";

        // Body (optional)
        ss << body;

        return ss.str();
    }
}