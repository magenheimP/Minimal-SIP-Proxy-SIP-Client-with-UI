#include <iostream>
#include "proxy/sip_proxy.hpp"

int main(int argc, char* argv[])
{
    uint16_t udp_port = 5060;

    uint16_t tcp_port = 0;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--udp-port" && i + 1 < argc) {
            udp_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--tcp-port" && i + 1 < argc) {
            tcp_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n"
                      << "Usage: " << argv[0]
                      << " [--udp-port <port>] [--tcp-port <port>]\n";
            return 1;
        }
    }

    proxy::SIPProxy proxy;

    proxy.start(udp_port, tcp_port);

    if (tcp_port > 0)
        std::cout << "Proxy running — UDP:" << udp_port
                  << "  TCP:" << tcp_port << "\n";
    else
        std::cout << "Proxy running — UDP:" << udp_port << "\n";

    std::cout << "Press ENTER to stop\n";
    std::cin.get();

    proxy.stop();
    return 0;
}
