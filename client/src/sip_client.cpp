#include "client/sip_client.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

SIPClient::SIPClient(const std::string& server_ip, int server_port)
    : server_ip_(server_ip)
    , server_port_(server_port)
    , logger_(common::Logger::instance())
    , transport_()
    , factory_(server_ip, server_port)
    , receiver_()
    , state_()
    , register_handler_(*this)
    , invite_handler_(*this)
    , cli_(*this)
{
    logger_.log("CLIENT", "-", "STARTUP", "SIPClient constructed");
}

void SIPClient::run()
{
    cli_.run();
}

void SIPClient::send_to_server(const std::string& message)
{
    transport_.send(message, server_ip_, static_cast<uint16_t>(server_port_));
}

bool SIPClient::wait_for_register_response(int timeout_seconds)
{
    return receiver_.wait_for_response(timeout_seconds);
}

void SIPClient::reset_receive_state()
{
    receiver_.reset();
}

std::pair<bool, std::string> SIPClient::register_response_snapshot() const
{
    return { receiver_.get_register_received(),
             receiver_.get_register_response() };
}

void SIPClient::set_pending_headers(const std::string& method,
                                     const std::string& headers)
{
    pending_headers_[method] = headers;
}

std::string SIPClient::pending_headers(const std::string& method) const
{
    auto it = pending_headers_.find(method);
    return it != pending_headers_.end() ? it->second : std::string{};
}

std::string SIPClient::build_register_message(const std::string& username,
                                               const std::string& domain,
                                               const std::string& extra_headers)
{
    return factory_.build_register(username, domain, extra_headers);
}

void SIPClient::do_register(const std::string& username,
                             const std::string& domain,
                             const std::string& extra_headers)
{
    set_pending_headers("REGISTER", extra_headers);
    register_handler_.handle_register(username, domain);
}

void SIPClient::do_invite(const std::string& from_user,
                           const std::string& from_domain,
                           const std::string& to_user,
                           const std::string& to_domain)
{

    invite_handler_.handle_invite(from_user, from_domain, to_user, to_domain);
}

void SIPClient::do_bye(const std::string& extra_headers)
{
    set_pending_headers("BYE", extra_headers);
    invite_handler_.handle_bye();
}

SIPClientStateManager& SIPClient::state()
{
    return state_;
}

common::Logger& SIPClient::logger()
{
    return logger_;
}


void SIPClient::on_packet_received(const std::string& data,
                                    const std::string& sender_ip,
                                    uint16_t           sender_port)
{
    logger_.log("NETWORK", "-", "RECV_PACKET",
                "From " + sender_ip + ":" + std::to_string(sender_port));
    logger_.log("NETWORK", "-", "SIP_MESSAGE", data);

    std::cout << "\nReceived SIP response from "
              << sender_ip << ":" << sender_port << "\n"
              << data << "\n> " << std::flush;

    receiver_.handle_receive(data);
    invite_handler_.on_message(data);

    if (call_state_cb_) {
        call_state_cb_(
            SIPClientStateManager::state_name(state_.get_state()),
            state_.active_call_id(),
            state_.active_remote_uri()
        );
    }

    if (register_response_cb_ && receiver_.get_register_received()) {
        const std::string response = receiver_.get_register_response();
        const bool success = response.find("SIP/2.0 200 OK") != std::string::npos;
        register_response_cb_(success, response);
    }
}

void SIPClient::notify_call_state_changed() {
    if (call_state_cb_) {
        call_state_cb_(
            SIPClientStateManager::state_name(state_.get_state()),
            state_.active_call_id(),
            state_.active_remote_uri()
        );
    }
}
void SIPClient::start_transport() {
    transport_.start(
        0,
        [this](const std::string& data,
               const std::string& sender_ip,
               uint16_t           sender_port)
        {
            on_packet_received(data, sender_ip, sender_port);
        }
    );

    factory_.set_local_port(transport_.local_port());

    logger_.log("NETWORK", "-", "READY",
        "UDP transport started on port "
        + std::to_string(transport_.local_port()));
}

void SIPClient::set_register_response_callback(ResponseCallback cb) {
    register_response_cb_ = std::move(cb);

}

void SIPClient::set_call_state_callback(StateCallback cb) {
    call_state_cb_ = std::move(cb);
}

void SIPClient::set_incoming_call_callback(
    std::function<void(const std::string&, const std::string&)> cb) {
    incoming_call_cb_ = std::move(cb);
}
const std::string& SIPClient::local_ip() const { return server_ip_; }
SIPMessageFactory& SIPClient::factory()         { return factory_; }

void SIPClient::do_answer() {
    invite_handler_.handle_answer();
}

void SIPClient::do_reject() {
    invite_handler_.handle_reject();
}
SIPClient::~SIPClient()
{
    transport_.stop();
    logger_.log("NETWORK", "-", "STOP", "UDP transport stopped");
    logger_.log("CLIENT", "-", "EXIT", "Program terminated");
}