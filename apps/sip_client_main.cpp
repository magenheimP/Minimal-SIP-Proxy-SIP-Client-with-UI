#include <QApplication>
#include <iostream>

#include "client/sip_client.hpp"
#include "ui/sip_qt_app.hpp"
#include "common/logger.hpp"

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "Usage: "
                  << argv[0]
                  << " <server_ip> <server_port> --cli|--ui\n";
        return 1;
    }

    const std::string server_ip   = argv[1];
    const int         server_port = std::stoi(argv[2]);
    const std::string mode        = argv[3];

    if (mode != "--cli" && mode != "--ui") {
        std::cerr << "Unknown mode: " << mode
                  << ". Use --cli or --ui\n";
        return 1;
    }

    common::Logger::instance("client.log")
        .log("MAIN", "-", "START",
             "Client starting in " + mode + " mode");

    try {
        SIPClient sip_client(server_ip, server_port);

        if (mode == "--cli") {
            sip_client.start_transport();
            sip_client.run();
        }
        else {
            QApplication app(argc, argv);
            SipQtApp qt_app(sip_client);

            sip_client.set_register_response_callback(
                [&qt_app](bool ok, const std::string& raw)
                {
                    qt_app.notify_register_response(ok, raw);
                }
            );

            sip_client.start_transport();
            qt_app.show();

            const int result = app.exec();

            common::Logger::instance()
                .log("CLIENT", "-", "EXIT", "Qt client exited");

            return result;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}