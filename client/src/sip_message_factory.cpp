#include "client/sip_message_factory.hpp"
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <regex>
#include <unordered_map>
#include <stdexcept>
#include <chrono>
#include <random>

SIPMessageFactory::SIPMessageFactory(const std::string& local_ip, int local_port)
    : local_ip_(local_ip)
    , local_port_(local_port)
    , cseq_(1)
    , rng_(std::chrono::steady_clock::now().time_since_epoch().count())
{
    std::uniform_int_distribution<uint64_t> dist;
    register_call_id_ = std::to_string(dist(rng_)) + "@" + local_ip_;
}

std::string SIPMessageFactory::new_call_id() const
{
    std::uniform_int_distribution<uint64_t> dist;
    return std::to_string(dist(rng_)) + "@" + local_ip_;
}

void SIPMessageFactory::set_local_port(uint16_t port)
{
    local_port_ = port;
}

std::string SIPMessageFactory::build(const std::string& method,
                                     const std::string& from_username,
                                     const std::string& from_domain,
                                     const std::string& to_username,
                                     const std::string& to_domain,
                                     const std::string& call_id,
                                     const std::string& body)
{
    if (method.empty())
        throw std::invalid_argument("SIP method must not be empty");

    const bool        is_register       = (method == "REGISTER");
    const std::string to_uri            = "sip:" + to_username + "@" + to_domain;
    const std::string effective_call_id = is_register ? register_call_id_ : call_id;

    common::SIPMessage msg;
    msg.start_line = method + " " + to_uri + " SIP/2.0";

    msg.add_header("Via",     make_via());
    msg.add_header("From",    "<sip:" + from_username + "@" + from_domain + ">");
    msg.add_header("To",      "<" + to_uri + ">");
    msg.add_header("Call-ID", effective_call_id);
    msg.add_header("CSeq",    std::to_string(cseq_++) + " " + method);

    if (is_register || method == "INVITE")
        msg.add_header("Contact", "<sip:" + from_username + "@" + local_ip_
                                  + ":" + std::to_string(local_port_) + ">");
    msg.add_header("Content-Length", body.empty() ? "0" : std::to_string(body.size()));

    if (!body.empty())
        msg.body = body;

    return serialize(msg);
}

std::string SIPMessageFactory::make_via() const
{
    return "SIP/2.0/UDP " + local_ip_ + ":" + std::to_string(local_port_);
}

std::string SIPMessageFactory::serialize(const common::SIPMessage& msg) const
{
    std::ostringstream oss;
    oss << msg.start_line << "\r\n";
    for (const auto& h : msg.headers)
        oss << h.name << ": " << h.value << "\r\n";
    oss << "\r\n" << msg.body;
    return oss.str();
}

std::string SIPMessageFactory::build_register(const std::string& username,
                                               const std::string& domain)
{
    return build("REGISTER", username, domain, username, domain);
}

std::string SIPMessageFactory::build_invite(const std::string& from_user,
                                             const std::string& from_domain,
                                             const std::string& to_user,
                                             const std::string& to_domain,
                                             const std::string& call_id)
{
    return build("INVITE", from_user, from_domain, to_user, to_domain, call_id);
}


std::string SIPMessageFactory::build_ack(const std::string& from_user,
                                          const std::string& from_domain,
                                          const std::string& to_user,
                                          const std::string& to_domain,
                                          const std::string& call_id)
{
    return build("ACK", from_user, from_domain, to_user, to_domain, call_id);
}


std::string SIPMessageFactory::build_ack_for_error(const std::string& from_user,
                                                    const std::string& from_domain,
                                                    const std::string& to_user,
                                                    const std::string& to_domain,
                                                    const std::string& call_id,
                                                    const std::string& error_response_raw)
{

    std::regex cseq_re(R"(CSeq\s*:\s*(\d+)\s+\w+)", std::regex::icase);
    std::smatch m;
    int cseq_num = 0;
    if (std::regex_search(error_response_raw, m, cseq_re))
        cseq_num = std::stoi(m[1].str());

    const std::string to_uri = "sip:" + to_user + "@" + to_domain;

    common::SIPMessage msg;
    msg.start_line = "ACK " + to_uri + " SIP/2.0";

    msg.add_header("Via",            make_via());
    msg.add_header("From",           "<sip:" + from_user + "@" + from_domain + ">");
    msg.add_header("To",             "<" + to_uri + ">");
    msg.add_header("Call-ID",        call_id);
    msg.add_header("CSeq",           std::to_string(cseq_num) + " ACK");
    msg.add_header("Content-Length", "0");


    return serialize(msg);
}

std::string SIPMessageFactory::build_bye(const std::string& from_user,
                                          const std::string& from_domain,
                                          const std::string& to_user,
                                          const std::string& to_domain,
                                          const std::string& call_id)
{
    return build("BYE", from_user, from_domain, to_user, to_domain, call_id);
}

std::string SIPMessageFactory::build_response(int code,
                                               const std::string& reason,
                                               const std::string& request_raw)
{
    auto get_header = [&](const std::string& name) -> std::string {
        std::regex re(name + R"(\s*:\s*(.+))", std::regex::icase);
        std::smatch m;
        if (std::regex_search(request_raw, m, re))
            return m[1].str();
        return {};
    };

    std::ostringstream oss;
    oss << "SIP/2.0 " << code << " " << reason << "\r\n";
    oss << "Via: "         << get_header("Via")     << "\r\n";
    oss << "From: "        << get_header("From")    << "\r\n";
    oss << "To: "          << get_header("To")      << "\r\n";
    oss << "Call-ID: "     << get_header("Call-ID") << "\r\n";
    oss << "CSeq: "        << get_header("CSeq")    << "\r\n";
    oss << "Content-Length: 0\r\n\r\n";
    return oss.str();
}