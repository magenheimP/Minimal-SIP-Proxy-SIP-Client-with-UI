#pragma once
#include <string>
#include <memory>

#include "common/logger.hpp"
#include "networking/udp_transport.hpp"
#include "client/sip_message_factory.hpp"
#include "client/sip_receive_handler.hpp"
#include "client/sip_client_state_manager.hpp"
#include "client/sip_register_handler.hpp"
#include "client/sip_invite_handler.hpp"
#include "client/cli_handler.hpp"

class SIPClient {
public:
    SIPClient(const std::string& server_ip, int server_port);
    ~SIPClient();

    void run();

    void send_to_server(const std::string& message);
    const std::string& local_ip() const;

    bool wait_for_register_response(int timeout_seconds);
    void reset_receive_state();

    using ResponseCallback = std::function<void(bool, const std::string&)>;
    void set_register_response_callback(ResponseCallback cb);
    void start_transport();
    
    std::pair<bool, std::string> register_response_snapshot() const;
    std::string build_register_message(const std::string& username,
                                       const std::string& domain);
    void do_register(const std::string& username, const std::string& domain);
    void do_reject();
    void do_answer();
    using StateCallback = std::function<void(const std::string& state,
                                              const std::string& call_id,
                                              const std::string& remote_uri)>;
    void set_call_state_callback(StateCallback cb);
    void set_incoming_call_callback(std::function<void(const std::string& call_id,
                                                        const std::string& caller)> cb);
    void do_invite(const std::string& from_user,
                   const std::string& from_domain,
                   const std::string& to_user,
                   const std::string& to_domain);
    void do_bye();
    void notify_call_state_changed();

    SIPClientStateManager& state();
    common::Logger&        logger();
    SIPMessageFactory&     factory();

private:

    ResponseCallback register_response_cb_;
    StateCallback call_state_cb_;
    std::function<void(const std::string&, const std::string&)> incoming_call_cb_;
    void on_packet_received(const std::string& data,
                            const std::string& sender_ip,
                            uint16_t           sender_port);

    std::string            server_ip_;
    int                    server_port_;

    common::Logger&        logger_;
    UdpTransport           transport_;
    SIPMessageFactory      factory_;
    SIPReceiveHandler      receiver_;
    SIPClientStateManager  state_;
    SIPRegisterHandler     register_handler_;
    SIPInviteHandler       invite_handler_;
    CLIHandler             cli_;
};