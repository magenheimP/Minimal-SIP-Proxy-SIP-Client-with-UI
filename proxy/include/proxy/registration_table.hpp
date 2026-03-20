//
// Created by tamara on 3/9/26.
//

#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <shared_mutex>

namespace proxy {

class RegistrationTable {
public:

    void register_user(const std::string& user,
                        const std::string& contact);

    std::optional<std::string> get_contact(const std::string& user) const;

    bool is_registered(const std::string& user) const;

private:

    std::unordered_map<std::string, std::string> registrations_;

    mutable std::shared_mutex mutex_;

    // TODO: protect registration table with mutex when proxy becomes multi-threaded
};

}
