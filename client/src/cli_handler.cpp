#include "client/cli_handler.hpp"
#include "client/sip_client.hpp"
#include <sstream>
#include <iostream>
#include <limits>

CLIHandler::CLIHandler(SIPClient& client)
    : client_(client)
{}

void CLIHandler::run()
{
    auto& log = client_.logger();
    bool running = true;

    while (running) {
        std::cout << "> " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) break;

        // Extracts only first word beacuse (something command will work and be unknown command at same time)
        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd) || cmd.empty()) continue;

        log.log("CLI", "-", "COMMAND", cmd);

        if (cmd == "register") {
            handle_register_command();

        } else if (cmd == "status") {
            if (client_.state().is_registered()) {
                std::cout << "Registered as: "
                          << client_.state().registered_user() << "\n";
            } else {
                std::cout << "Not registered.\n";
            }

        } else if (cmd == "exit" || cmd == "quit") {
            log.log("CLIENT", "-", "SHUTDOWN", "User requested exit");
            std::cout << "Exiting.\n";
            running = false;

        } else {
            log.log("CLI", "-", "UNKNOWN_COMMAND", cmd);
            std::cout << "Unknown command: " << cmd
                      << "  (try: register, status, exit)\n";
        }
    }
}

bool CLIHandler::prompt(const std::string& prompt_text, std::string& out) const
{
    std::cout << prompt_text;
    if (!(std::cin >> out) || out.empty()) {
        std::cerr << "Invalid input.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

void CLIHandler::handle_register_command()
{
    std::string username, domain;
    if (!prompt("username: ", username)) return;
    if (!prompt("domain: ",   domain))   return;

    client_.do_register(username, domain);
}
