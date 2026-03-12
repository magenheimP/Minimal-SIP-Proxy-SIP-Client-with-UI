#include "client/sip_message_factory.hpp"
#include "common/sip_message.hpp"

#include <sstream>
#include <ctime>
#include <cstdlib>

SIPMessageFactory::SIPMessageFactory(const std::string& local_ip, int local_port)
    : local_ip_(local_ip), local_port_(local_port), cseq_(1)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    call_id_ = std::to_string(std::rand() % 1000000) + "@" + local_ip_;
}

std::string SIPMessageFactory::build_register(const std::string& username,
                                               const std::string& domain)
{
    common::SIPMessage msg;
    msg.start_line = "REGISTER sip:" + domain + " SIP/2.0";

    const std::string port_str = std::to_string(local_port_);

    msg.add_header("Via",            "SIP/2.0/UDP " + local_ip_ + ":" + port_str);
    msg.add_header("From",           "<sip:" + username + "@" + domain + ">");
    msg.add_header("To",             "<sip:" + username + "@" + domain + ">");
    msg.add_header("Call-ID",        call_id_);
    msg.add_header("CSeq",           std::to_string(cseq_) + " REGISTER");
    msg.add_header("Contact",        "<sip:" + username + "@" + local_ip_ + ">");
    msg.add_header("Content-Length", "0");

    ++cseq_;

    return serialize(msg);
}



std::string SIPMessageFactory::serialize(const common::SIPMessage& msg) const
{
    std::ostringstream oss;
    oss << msg.start_line << "\r\n";
    for (const auto& h : msg.headers) {
        oss << h.name << ": " << h.value << "\r\n";
    }
    oss << "\r\n";
    return oss.str();
}
