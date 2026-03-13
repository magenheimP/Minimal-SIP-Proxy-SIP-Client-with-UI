//
// Created by lazarstani on 3/12/26.
//
#include <iostream>

#include "../../common/include/common/sip_message.hpp"
#include "../../proxy/include/proxy/sip_parser.hpp"
#include "../../proxy/include/proxy/register_handler.hpp"
#include "../../proxy/include/proxy/registration_table.hpp"
#include "../../common/include/common/logger.hpp"
#include "../../common/include/common/types.hpp"
#include "../include/networking/udp_transport.hpp"
#include "../include/networking/thread_pool.hpp"
#include "../include/networking/dispatcher.hpp"

int main() {

    common::Logger::instance("sip_proxy.log")
        .log("MAIN", "-", "START", "Proxy starting");

    ThreadPool pool(4);
    UdpTransport transport;

    proxy::RegistrationTable table;
    proxy::RegisterHandler handler(table);

    Dispatcher dispatcher(pool,
        [&](const common::RawPacket& pkt) {

            common::Logger::instance().log(
                "NETWORK",
                "-",
                "RECV",
                "Packet from " + pkt.ip + ":" + std::to_string(pkt.port)
            );

            common::SIPMessage request = proxy::SIPParser::parse(pkt.data);

            common::SIPMessage response = handler.handle(request);// later -> auto response = router.route(request, pkt.ip, pkt.port);

            transport.send(response.serialize(), pkt.ip, pkt.port);

            common::Logger::instance().log(
                "NETWORK",
                response.get_header("Call-ID"),
                "SEND",
                "Response sent to " + pkt.ip
            );
        }
    );

    dispatcher.start();

    try {
        transport.start(5060,
            [&](const std::string& data,
                const std::string& ip,
                uint16_t port)
            {
                dispatcher.push(common::RawPacket{data, ip, port});
            });
    }
    catch (const std::exception& e) {
        std::cerr << "Transport start failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "UDP server running on port 5060\n";
    std::cout << "Press ENTER to stop\n";

    common::Logger::instance().log(
        "NETWORK",
        "-",
        "INFO",
        "UDP server running on port 5060"
    );

    std::cin.get();

    common::Logger::instance().log(
        "MAIN",
        "-",
        "STOP",
        "Proxy shutting down"
    );

    transport.stop();
    dispatcher.stop();
    pool.shutdown();
}

