#pragma once
#include <cstdint>
#include <string>
#include "common/sip_message.hpp"

class SIPMessageFactory {
public:
    SIPMessageFactory(const std::string& local_ip, int local_port);
    void set_local_port(uint16_t port);
    std::string build(const std::string& method,
                    const std::string& from_username,
                    const std::string& from_domain,
                    const std::string& to_username,
                    const std::string& to_domain,
                    const std::string& call_id      = {},
                    const std::string& body         = {},
                    const std::string& extra_headers = {});
    std::string build_ack_for_error(const std::string& from_user,
                                                    const std::string& from_domain,
                                                    const std::string& to_user,
                                                    const std::string& to_domain,
                                                    const std::string& call_id,
                                                    const std::string& error_response_raw);
    std::string new_call_id() const;
    std::string build_register(const std::string& username,
                            const std::string& domain,
                            const std::string& extra_headers = {});

    std::string build_invite(const std::string& from_user,
                              const std::string& from_domain,
                              const std::string& to_user,
                              const std::string& to_domain,
                              const std::string& call_id,
                              const std::string& extra_headers = {});

    std::string build_ack(const std::string& from_user,
                           const std::string& from_domain,
                           const std::string& to_user,
                           const std::string& to_domain,
                           const std::string& call_id,
                           const std::string& extra_headers = {});

    std::string build_bye(const std::string& from_user,
                           const std::string& from_domain,
                           const std::string& to_user,
                           const std::string& to_domain,
                           const std::string& call_id,
                           const std::string& extra_headers = {});
    std::string build_response(int code,
                                               const std::string& reason,
                                               const std::string& request_raw);

private:
    std::string serialize(const common::SIPMessage& msg) const;
    std::string make_via() const;

    std::string local_ip_;
    int         local_port_;
    int         cseq_;
    std::string register_call_id_;
};