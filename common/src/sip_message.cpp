#include "common/sip_message.h"

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

}