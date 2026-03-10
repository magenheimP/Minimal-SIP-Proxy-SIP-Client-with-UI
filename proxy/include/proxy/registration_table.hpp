//
// Created by tamara on 3/9/26.
//

#pragma once

#include <string>
#include <unordered_map>

namespace proxy {

class RegistrationTable {
public:

  void register_user(const std::string& user,
                     const std::string& contact);

  std::string get_contact(const std::string& user) const;

private:

  std::unordered_map<std::string, std::string> registrations_;

  // TODO: protect registration table with mutex when proxy becomes multi-threaded
};

}
