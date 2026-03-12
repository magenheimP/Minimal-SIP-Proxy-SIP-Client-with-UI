#include "client/sip_client.hpp"
#include <iostream>

int main()
{
    try {
        common::Logger::instance("client.log")
               .log("MAIN", "-", "START", "Client starting");
        SIPClient client("127.0.0.1", 5060);
        client.run();

    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
