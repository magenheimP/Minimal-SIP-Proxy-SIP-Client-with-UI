#include "client/sip_invite_handler.hpp"
#include "client/sip_client.hpp"
#include <iostream>
#include <regex>
#include <algorithm>

SIPInviteHandler::SIPInviteHandler(SIPClient& client)
    : client_(client)
{}


void SIPInviteHandler::handle_invite(const std::string& from_user,
                                      const std::string& from_domain,
                                      const std::string& to_user,
                                      const std::string& to_domain)
{
    auto& state = client_.state();

    if (!state.is_registered()) {
        std::cout << "Error: must register before placing a call.\n";
        return;
    }

    if (state.is_calling() || state.is_ringing() || state.is_in_call()) {
        std::cout << "Error: already in a call. Hang up first.\n";
        return;
    }

    const std::string call_id    = client_.factory().new_call_id();
    const std::string remote_uri = "sip:" + to_user + "@" + to_domain;

    state.on_calling(call_id, remote_uri);

    const std::string msg =
        client_.factory().build_invite(from_user, from_domain,
                                        to_user,   to_domain, call_id);

    client_.logger().log("CALL", call_id, "INVITE_SENT", remote_uri);
    client_.send_to_server(msg);
    std::cout << "INVITE sent to " << remote_uri
              << "  [call-id: " << call_id << "]\n";
}


void SIPInviteHandler::handle_bye()
{
    auto& state = client_.state();

    if (!state.is_in_call()) {
        std::cout << "No active call to hang up.\n";
        return;
    }

    const std::string call_id    = state.active_call_id();
    const std::string remote_uri = state.active_remote_uri();

    const std::string local = state.registered_user(); // "user@domain"
    const auto at1           = local.find('@');
    const std::string from_user   = local.substr(0, at1);
    const std::string from_domain = local.substr(at1 + 1);

    auto [to_user, to_domain] = split_sip_uri(remote_uri);

    const std::string msg =
        client_.factory().build_bye(from_user, from_domain,
                                     to_user,   to_domain, call_id);

    client_.logger().log("CALL", call_id, "BYE_SENT", remote_uri);
    client_.send_to_server(msg);
    std::cout << "BYE sent  [call-id: " << call_id << "]\n";
}


void SIPInviteHandler::on_message(const std::string& raw)
{
    std::string upper = raw;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper.rfind("SIP/2.0", 0) == std::string::npos) return;

    // Only process if the call id matches our active call
    const std::string msg_call_id = extract_call_id(raw);
    if (msg_call_id.empty()) return;
    if (msg_call_id != client_.state().active_call_id()) return;

    const int code = extract_status_code(raw);

    switch (code) {
        case 100: handle_100();          break;
        case 180: handle_180();          break;
        case 200:

            if (client_.state().is_in_call())
                handle_200_ok_bye();
            else
                handle_200_ok_invite();
            break;
        default:
        if (code >= 400) handle_error(code);
        break;
    }
}


void SIPInviteHandler::handle_100()
{
    const std::string call_id = client_.state().active_call_id();
    client_.state().transition_to_unlocked(SIPClientStateManager::State::Calling);
    client_.logger().log("CALL", call_id, "100_TRYING", "");
    std::cout << "[" << call_id << "] 100 Trying\n> " << std::flush;
}

void SIPInviteHandler::handle_180()
{
    const std::string call_id = client_.state().active_call_id();
    client_.state().on_ringing();
    client_.logger().log("CALL", call_id, "180_RINGING", "");
    std::cout << "[" << call_id << "] 180 Ringing...\n> " << std::flush;
}

void SIPInviteHandler::handle_200_ok_invite()
{
    auto& state = client_.state();
    const std::string call_id    = state.active_call_id();
    const std::string remote_uri = state.active_remote_uri();

    state.on_call_established();
    client_.logger().log("CALL", call_id, "200_OK_INVITE", "sending ACK");
    std::cout << "[" << call_id << "] 200 OK – sending ACK. Call active.\n> "
              << std::flush;

    const std::string local = state.registered_user();
    const auto at = local.find('@');
    const std::string from_user   = local.substr(0, at);
    const std::string from_domain = local.substr(at + 1);
    auto [to_user, to_domain] = split_sip_uri(remote_uri);

    const std::string ack =
        client_.factory().build_ack(from_user, from_domain,
                                     to_user,   to_domain, call_id);
    client_.send_to_server(ack);
}

void SIPInviteHandler::handle_200_ok_bye()
{
    const std::string call_id = client_.state().active_call_id();
    client_.state().on_call_terminated();
    client_.logger().log("CALL", call_id, "CALL_TERMINATED", "");
    std::cout << "[" << call_id << "] Call terminated.\n> " << std::flush;
}


std::string SIPInviteHandler::extract_call_id(const std::string& raw)
{
    std::regex re(R"((?:Call-ID|i)\s*:\s*(\S+))", std::regex::icase);
    std::smatch m;
    if (std::regex_search(raw, m, re)) return m[1].str();
    return {};
}

int SIPInviteHandler::extract_status_code(const std::string& raw)
{
    std::regex re(R"(SIP/2\.0\s+(\d{3}))");
    std::smatch m;
    if (std::regex_search(raw, m, re)) return std::stoi(m[1].str());
    return 0;
}

std::pair<std::string, std::string>
SIPInviteHandler::split_sip_uri(const std::string& uri)
{
    // Handles "sip:user@domain" and "<sip:user@domain>"
    std::regex re(R"(sip:([^@>]+)@([^>;>\s]+))", std::regex::icase);
    std::smatch m;
    if (std::regex_search(uri, m, re))
        return { m[1].str(), m[2].str() };
    return {};
}

void SIPInviteHandler::handle_error(int code)
{
    const std::string call_id = client_.state().active_call_id();
    client_.logger().log("CALL", call_id, "ERROR", std::to_string(code));
    std::cout << "[" << call_id << "] Call failed: " << code << "\n> " << std::flush;
    client_.state().on_call_terminated();
}