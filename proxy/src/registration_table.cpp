//
// Created by tamara on 3/9/26.
//

#include "proxy/registration_table.hpp"

namespace proxy {

void RegistrationTable::register_user(const std::string& user,
                                      const std::string& contact) {
  registrations_[user] = contact;
}

std::string RegistrationTable::get_contact(const std::string& user) const {
  auto it = registrations_.find(user);
  if (it != registrations_.end()) {
    return it->second;
  }
  return "";
}

}
