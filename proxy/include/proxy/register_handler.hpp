//
// Created by tamara on 3/9/26.
//

#pragma once

#include "../../common/include/common/sip_message.hpp"
#include "../proxy/registration_table.hpp"

namespace proxy {

class RegisterHandler {
public:

    explicit RegisterHandler(RegistrationTable& table);

    common::SIPMessage handle(const common::SIPMessage& message) const;

private:

    RegistrationTable& table_;
};

}
