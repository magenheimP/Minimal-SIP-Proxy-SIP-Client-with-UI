#pragma once
#include <string>
#include "common/sip_message.hpp"

class SIPMessageFactory {
public:
    SIPMessageFactory(const std::string& local_ip, int local_port);

    std::string build(const std::string& method,
                  const std::string& from_username,
                  const std::string& from_domain,
                  const std::string& to_username,
                  const std::string& to_domain,
                  const std::string& call_id = {},
                  const std::string& body    = {});

    std::string build_register(const std::string& username,
                                               const std::string& domain);

private:
    std::string serialize(const common::SIPMessage& msg) const;
    std::string make_via() const;

    std::string local_ip_;
    int         local_port_;
    int         cseq_;
    std::string register_call_id_;
};