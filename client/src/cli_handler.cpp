#include "client/cli_handler.hpp"
#include "client/sip_client.hpp"
#include <sstream>
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>
CLIHandler::CLIHandler(SIPClient& client)
    : client_(client)
{}
void CLIHandler::handle_sleep_command(std::istringstream& iss)
{
    int seconds;
    if (!(iss >> seconds)) {
        std::cout << "Usage: sleep <seconds>\n";
        return;
    }
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

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
        } else if (cmd == "call") {
            handle_call_command();
        } else if (cmd == "hangup") {
            client_.do_bye();
        }else if (cmd == "answer") {
            client_.do_answer();
        } else if (cmd == "reject") {
            client_.do_reject();
        } else if (cmd == "sleep") {
            handle_sleep_command(iss);
        }else if (cmd == "help") {
            std::cout << "Commands: register, call, hangup, status, exit\n";
            std::cout << "Commands: register, call, hangup, status, sleep, exit\n";
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

void CLIHandler::handle_call_command()
{
    const std::string reg = client_.state().registered_user();
    if (reg.empty()) { std::cout << "Not registered.\n"; return; }

    const auto at       = reg.find('@');
    const std::string from_user   = reg.substr(0, at);
    const std::string from_domain = reg.substr(at + 1);

    std::string to_user, to_domain;
    if (!prompt("callee username: ", to_user))   return;
    if (!prompt("callee domain: ",   to_domain)) return;

    client_.do_invite(from_user, from_domain, to_user, to_domain);
}