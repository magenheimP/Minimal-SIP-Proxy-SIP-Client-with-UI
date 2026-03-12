#pragma once
#include <string>
#include <memory>

#include "common/logger.hpp"
#include "networking/udp_transport.hpp"
#include "client/sip_message_factory.hpp"
#include "client/sip_receive_handler.hpp"
#include "client/sip_state_manager.hpp"
#include "client/sip_register_handler.hpp"
#include "client/cli_handler.hpp"


class SIPClient {
public:
    SIPClient(const std::string& server_ip, int server_port);
    ~SIPClient() = default;

    void run();

    void send_to_server(const std::string& message);

    bool wait_for_register_response(int timeout_seconds);

    void reset_receive_state();

    std::pair<bool, std::string> register_response_snapshot() const;

    std::string build_register_message(const std::string& username,
                                       const std::string& domain);

    void do_register(const std::string& username, const std::string& domain);

    SIPStateManager& state();

    common::Logger& logger();

private:
    void on_packet_received(const std::string& data,
                            const std::string& sender_ip,
                            uint16_t           sender_port);

    std::string server_ip_;
    int         server_port_;

    common::Logger&      logger_;
    UdpTransport         transport_;
    SIPMessageFactory    factory_;
    SIPReceiveHandler    receiver_;
    SIPStateManager      state_;
    SIPRegisterHandler   register_handler_;
    CLIHandler           cli_;
};
