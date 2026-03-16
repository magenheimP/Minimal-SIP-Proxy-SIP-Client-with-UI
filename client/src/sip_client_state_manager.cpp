#include "client/sip_client_state_manager.hpp"

SIPClientStateManager::State SIPClientStateManager::get_state() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return state_;
}

bool SIPClientStateManager::is_registered() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return state_ == State::Registered;
}

std::string SIPClientStateManager::registered_user() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return registered_user_;
}

void SIPClientStateManager::on_registering()
{
    transition_to(State::Registering);
}

void SIPClientStateManager::on_register_success(const std::string& username,
                                                  const std::string& domain)
{
    std::lock_guard<std::mutex> lock(mtx_);
    registered_user_ = username + "@" + domain;
    transition_to_unlocked(State::Registered);
}

void SIPClientStateManager::on_register_failed()
{
    transition_to(State::Failed);
}

void SIPClientStateManager::transition_to(State s)
{
    std::lock_guard<std::mutex> lock(mtx_);
    transition_to_unlocked(s);
}

void SIPClientStateManager::transition_to_unlocked(State s)
{
    state_ = s;
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

std::string SIPClientStateManager::active_call_id() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return active_call_id_;
}
std::string SIPClientStateManager::active_remote_uri() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return active_remote_uri_;
}

void SIPClientStateManager::on_calling(const std::string& call_id,
                                        const std::string& remote_uri)
{
    std::lock_guard<std::mutex> lock(mtx_);
    active_call_id_    = call_id;
    active_remote_uri_ = remote_uri;
    transition_to_unlocked(State::Calling);
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
    transition_to_unlocked(State::Registered);
}