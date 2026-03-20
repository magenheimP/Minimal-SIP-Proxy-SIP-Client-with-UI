#include <iostream>

#include "proxy/sip_proxy.hpp"

int main()
{
    proxy::SIPProxy proxy;

    proxy.start(5060);

    std::cout << "Press ENTER to stop\n";
    std::cin.get();

    proxy.stop();
}