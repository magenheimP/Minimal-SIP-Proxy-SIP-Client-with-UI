#include "client/sip_client_state_manager.hpp"
#include "common/logger.hpp"

SIPClientStateManager::State SIPClientStateManager::get_state() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return state_;
}

bool SIPClientStateManager::is_registered() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return state_ == State::Registered;
}

std::string SIPClientStateManager::registered_user() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return registered_user_;
}

void SIPClientStateManager::on_registering() {
    transition_to(State::Registering);
}

void SIPClientStateManager::on_register_success(const std::string& username,
                                                  const std::string& domain) {
    std::lock_guard<std::mutex> lock(mtx_);
    registered_user_ = username + "@" + domain;
    transition_to_unlocked(State::Registered);
}

void SIPClientStateManager::on_register_failed() {
    transition_to(State::Failed);
}

void SIPClientStateManager::transition_to(State s) {
    std::lock_guard<std::mutex> lock(mtx_);
    transition_to_unlocked(s);
}

std::string SIPClientStateManager::state_name(State s)
{
    switch (s) {
    case State::Idle:        return "Idle";
    case State::Registering: return "Registering";
    case State::Registered:  return "Registered";
    case State::Calling:     return "Calling";
    case State::Ringing:     return "Ringing";
    case State::InCall:      return "InCall";
    case State::Failed:      return "Failed";
    }
    return "Unknown";
}

void SIPClientStateManager::transition_to_unlocked(State s)
{

    if (s == state_) return;

    const std::string from = state_name(state_);
    const std::string to   = state_name(s);
    state_ = s;


    logger_.log("STATE", active_call_id_.empty() ? "-" : active_call_id_,
                "TRANSITION", from + " -> " + to);
}

bool SIPClientStateManager::is_in_call() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return state_ == State::InCall;
}

bool SIPClientStateManager::is_ringing() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return state_ == State::Ringing;
}

bool SIPClientStateManager::is_calling() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return state_ == State::Calling;
}

bool SIPClientStateManager::is_busy() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return state_ == State::Calling
        || state_ == State::Ringing
        || state_ == State::InCall;
}

std::string SIPClientStateManager::active_call_id() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return active_call_id_;
}

std::string SIPClientStateManager::active_remote_uri() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return active_remote_uri_;
}

void SIPClientStateManager::on_calling(const std::string& call_id,
                                        const std::string& remote_uri) {
    std::lock_guard<std::mutex> lock(mtx_);
    active_call_id_    = call_id;
    active_remote_uri_ = remote_uri;
    is_incoming_call_  = false;
    transition_to_unlocked(State::Calling);
}


void SIPClientStateManager::on_incoming_call(const std::string& call_id,
                                              const std::string& remote_uri) {
    std::lock_guard<std::mutex> lock(mtx_);
    active_call_id_    = call_id;
    active_remote_uri_ = remote_uri;
    is_incoming_call_  = true;
    transition_to_unlocked(State::Ringing);
}

void SIPClientStateManager::on_ringing() {
    transition_to(State::Ringing);
}

void SIPClientStateManager::on_call_established() {
    transition_to(State::InCall);
}

void SIPClientStateManager::on_call_terminated() {
    std::lock_guard<std::mutex> lock(mtx_);
    active_call_id_.clear();
    active_remote_uri_.clear();
    is_incoming_call_  = false;
    transition_to_unlocked(State::Registered);
}

bool SIPClientStateManager::is_incoming_call() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return is_incoming_call_;
}