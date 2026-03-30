//
// Created by tamara on 3/9/26.
//

#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <shared_mutex>

#include "../../common/include/common/types.hpp"

namespace proxy {

    struct ContactEntry {
        std::string contact;
        common::TransportType transport = common::TransportType::UDP;
    };

class RegistrationTable {
public:

    void register_user(const std::string& user,
                    const std::string& contact,
                    common::TransportType transport = common::TransportType::UDP);

    std::optional<std::string> get_contact(const std::string& user) const;

    std::optional<ContactEntry> get_contact_entry(const std::string& user) const;

    bool is_registered(const std::string& user) const;

private:

    std::unordered_map<std::string, ContactEntry> registrations_;

    mutable std::shared_mutex mutex_;
};

}
