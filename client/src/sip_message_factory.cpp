#include "client/sip_message_factory.hpp"
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <unordered_map>
#include <stdexcept>

SIPMessageFactory::SIPMessageFactory(const std::string& local_ip, int local_port)
    : local_ip_(local_ip)
    , local_port_(local_port)
    , cseq_(1)

{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    register_call_id_ = std::to_string(std::rand() % 1000000) + "@" + local_ip_;
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

    const bool        is_register      = (method == "REGISTER");
    const std::string to_uri           = "sip:" + to_username + "@" + to_domain;
    const std::string effective_call_id = is_register ? register_call_id_ : call_id;

    common::SIPMessage msg;
    msg.start_line = method + " " + to_uri + " SIP/2.0";

    msg.add_header("Via",     make_via());
    msg.add_header("From",    "<sip:" + from_username + "@" + from_domain + ">");
    msg.add_header("To",      "<" + to_uri + ">");
    msg.add_header("Call-ID", effective_call_id);
    msg.add_header("CSeq",    std::to_string(cseq_++) + " " + method);

    if (is_register)
        msg.add_header("Contact", "<sip:" + from_username + "@" + local_ip_ + ">");

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