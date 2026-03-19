#pragma once
#include <string>
#include <utility>

class SIPClient;

class SIPInviteHandler {
public:
    explicit SIPInviteHandler(SIPClient& client);


    void handle_invite(const std::string& from_user, const std::string& from_domain,
                       const std::string& to_user,   const std::string& to_domain);
    void handle_bye();
    void handle_answer();
    void handle_reject();


    void on_message(const std::string& raw);

private:

    void handle_incoming_invite(const std::string& raw);
    void handle_incoming_bye(const std::string& raw);


    void handle_100();
    void handle_180();
    void handle_200_ok_invite();
    void handle_200_ok_bye();
    void handle_error(int code, const std::string& raw);


    static std::string extract_call_id(const std::string& raw);
    static int         extract_status_code(const std::string& raw);
    static std::string extract_cseq_method(const std::string& raw);
    static std::pair<std::string, std::string> split_sip_uri(const std::string& uri);

    SIPClient&  client_;
    std::string pending_invite_;
};