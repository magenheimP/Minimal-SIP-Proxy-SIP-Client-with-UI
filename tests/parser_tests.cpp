//
// Created by tamara on 3/6/26.
//

#include <iostream>
#include "common/sip_message.hpp"

int main() {

    common::SIPMessage msg;

    msg.start_line = "REGISTER sip:localhost SIP/2.0";

    msg.add_header("Via", "SIP/2.0/UDP 127.0.0.1:5060");
    msg.add_header("From", "<sip:alice@localhost>");
    msg.add_header("To", "<sip:alice@localhost>");

    std::cout << "Start line: " << msg.start_line << std::endl;

    std::cout << "Via header: "
              << msg.get_header("Via") << std::endl;

    msg.set_header("Via", "SIP/2.0/UDP 192.168.1.10");

    std::cout << "Modified Via: "
              << msg.get_header("Via") << std::endl;

    return 0;
}