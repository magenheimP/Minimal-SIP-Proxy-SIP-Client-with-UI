#include <QApplication>
#include <iostream>

#include "client/sip_client.hpp"
#include "ui/sip_qt_app.hpp"
#include "common/logger.hpp"

int main(int argc, char* argv[])
{
    // Required args: <server_ip> <server_port> --cli|--ui
    // Optional:      --user <username> --domain <domain>

    if (argc < 4) {
        std::cerr << "Usage: "
                  << argv[0]
                  << " <server_ip> <server_port> --cli|--ui"
                  << " [--user <username>] [--domain <domain>]\n";
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


    std::string auto_username;
    std::string auto_domain;

    for (int i = 4; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--user" && i + 1 < argc) {
            auto_username = argv[++i];
        } else if (arg == "--domain" && i + 1 < argc) {
            auto_domain = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }


    if (auto_username.empty() != auto_domain.empty()) {
        std::cerr << "Both --user and --domain must be provided together.\n";
        return 1;
    }

    const bool auto_register = !auto_username.empty();

    common::Logger::instance("client.log")
        .log("MAIN", "-", "START",
             "Client starting in " + mode + " mode");

    try {
        SIPClient sip_client(server_ip, server_port);

        if (mode == "--cli") {
            sip_client.start_transport();

            if (auto_register) {
                std::cout << "Auto-registering as "
                          << auto_username << "@" << auto_domain << "\n";
                sip_client.do_register(auto_username, auto_domain);
            }

            sip_client.run();
        }
        else {
            QApplication app(argc, argv);
            SipQtApp qt_app(sip_client);

            sip_client.set_register_response_callback(
                [&qt_app](bool ok, const std::string& raw) {
                    qt_app.notify_register_response(ok, raw);
                });
            sip_client.set_call_state_callback(
                [&qt_app](const std::string& state,
                          const std::string& call_id,
                          const std::string& remote_uri) {
                    qt_app.notify_call_state(state, call_id, remote_uri);
                });
            sip_client.set_incoming_call_callback(
                [&qt_app](const std::string& call_id, const std::string& caller) {
                    qt_app.notify_incoming_call(call_id, caller);
                });

            sip_client.start_transport();

            if (auto_register) {
                std::cout << "Auto-registering as "
                          << auto_username << "@" << auto_domain << "\n";
                sip_client.do_register(auto_username, auto_domain);
            }

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