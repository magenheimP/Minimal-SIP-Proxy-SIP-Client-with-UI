//
// Created by tamara on 3/9/26.
//

#include "../include/proxy/registration_table.hpp"
#include "../../common/include/common/logger.hpp"

#include <mutex>
#include <shared_mutex>

namespace proxy {

void RegistrationTable::register_user(const std::string& user,
                                      const std::string& contact,
                                      common::TransportType transport) {
    std::unique_lock lock(mutex_);

    registrations_[user] = ContactEntry{contact, transport};

    common::Logger::instance("proxy.log").log(
        "RegistrationTable",
        "",
        "REGISTERED",
        "Stored registration for user = " + user + ", contact = " + contact);
}

std::optional<std::string> RegistrationTable::get_contact(const std::string& user) const {

    //  shared_lock for read operations
    std::shared_lock lock(mutex_);

    auto it = registrations_.find(user);
    if (it != registrations_.end()) {
        common::Logger::instance("proxy.log").log(
            "RegistrationTable",
            "",
            "LOOKUP_OK",
            "Found contact for user = " + user + ", contact = " + it->second.contact);

        return it->second.contact;
    }

    common::Logger::instance("proxy.log").log(
        "RegistrationTable",
        "",
        "LOOKUP_FAIL",
        "User not found in registration table: user = " + user);

    return std::nullopt;
}

std::optional<ContactEntry> RegistrationTable::get_contact_entry(const std::string& user) const {
    std::shared_lock lock(mutex_);

    auto it = registrations_.find(user);
    if (it != registrations_.end())
        return it->second;

    return std::nullopt;
}

bool RegistrationTable::is_registered(const std::string& user) const {
    std::shared_lock lock(mutex_);
    return registrations_.find(user) != registrations_.end();
}

}
