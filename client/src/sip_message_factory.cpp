#include "client/sip_message_factory.hpp"
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <unordered_map>
#include <stdexcept>

static const std::unordered_map<common::SIPRequest::Method, std::string> k_method_str = {
    { common::SIPRequest::Method::REGISTER, "REGISTER" },
    { common::SIPRequest::Method::INVITE,   "INVITE"   },
    { common::SIPRequest::Method::BYE,      "BYE"      },
    { common::SIPRequest::Method::ACK,      "ACK"      },
};

SIPMessageFactory::SIPMessageFactory(const std::string& local_ip, int local_port)
    : local_ip_(local_ip)
    , local_port_(local_port)
    , cseq_(1)

{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    register_call_id_ = std::to_string(std::rand() % 1000000) + "@" + local_ip_;
}

std::string SIPMessageFactory::build(const common::SIPRequest& req)
{
    const auto it = k_method_str.find(req.method);
    if (it == k_method_str.end())
        throw std::invalid_argument("Unknown SIP method");

    const std::string& method = it->second;
    const std::string  to_uri = "sip:" + req.to_username + "@" + req.to_domain;
    const std::string  call_id = (req.method == common::SIPRequest::Method::REGISTER)
                                     ? register_call_id_
                                     : req.call_id;

    common::SIPMessage msg;
    msg.start_line = method + " " + to_uri + " SIP/2.0";

    msg.add_header("Via",     make_via());
    msg.add_header("From",    "<sip:" + req.from_username + "@" + req.from_domain + ">");
    msg.add_header("To",      "<" + to_uri + ">");
    msg.add_header("Call-ID", call_id);
    msg.add_header("CSeq",    std::to_string(cseq_++) + " " + method);
    if (req.method == common::SIPRequest::Method::REGISTER)
        msg.add_header("Contact", req.contact);
    if (!req.body.empty()) {
        msg.add_header("Content-Length", std::to_string(req.body.size()));
        msg.body = req.body;
    } else {
        msg.add_header("Content-Length", "0");
    }

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
    common::SIPRequest req;
    req.method        = common::SIPRequest::Method::REGISTER;
    req.from_username = username;
    req.from_domain   = domain;
    req.to_username   = username;
    req.to_domain     = domain;
    req.contact       = "<sip:" + username + "@" + local_ip_ + ">";
    return build(req);
}