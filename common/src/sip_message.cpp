#include "../include/common/sip_message.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace common {

    // Helper for case-insensitive string comparison
    static bool headers_equal(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    }

    std::string SIPMessage::get_header(const std::string& name) const {
        for (const auto& h : headers) {
            if (headers_equal(h.name, name)) {
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
            if (headers_equal(h.name, name)) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> SIPMessage::get_headers(const std::string& name) const {
        std::vector<std::string> values;

        for (const auto& h : headers) {
            if (headers_equal(h.name, name)) {
                values.push_back(h.value);
            }
        }

        return values;
    }

    void SIPMessage::prepend_header(const std::string& name,
                                const std::string& value) {
        headers.insert(headers.begin(), {name, value});
    }

    bool SIPMessage::remove_first_header(const std::string& name) {
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            if (headers_equal(it->name, name)) {
                headers.erase(it);
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
            if (headers_equal(h.name, name)) {
                h.value = value;
                return;
            }
        }

        add_header(name, value);
    }

    std::string SIPMessage::get_method() const {
        if (!is_request()) {
            return "";
        }

        const std::size_t space_pos = start_line.find(' ');
        if (space_pos == std::string::npos) {
            return start_line;
        }

        return start_line.substr(0, space_pos);
    }

    int SIPMessage::get_status_code() const {
        if (!is_response()) {
            return -1;
        }

        std::istringstream ss(start_line);
        std::string version;
        int status_code = -1;

        ss >> version >> status_code;

        if (ss.fail()) {
            return -1;
        }

        return status_code;
    }

    void SIPMessage::update_content_length() {
        set_header("Content-Length", std::to_string(body.size()));
    }

    std::string SIPMessage::serialize() const {
        std::ostringstream ss;

        SIPMessage message_copy = *this;
        message_copy.update_content_length();

        // Start line
        ss << message_copy.start_line << "\r\n";

        // Headers
        for (const auto& header : message_copy.headers) {
            ss << header.name << ": " << header.value << "\r\n";
        }

        // Empty line separating headers and body
        ss << "\r\n";

        // Body (optional)
        ss << message_copy.body;

        return ss.str();
    }
}