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