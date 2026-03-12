#include "client/sip_state_manager.hpp"

SIPStateManager::RegistrationState SIPStateManager::get_registration_state() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return reg_state_;
}

void SIPStateManager::set_registration_state(RegistrationState s)
{
    std::lock_guard<std::mutex> lock(mtx_);
    reg_state_ = s;
}

std::string SIPStateManager::registered_user() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return registered_user_;
}

void SIPStateManager::set_registered_user(const std::string& user_at_domain)
{
    std::lock_guard<std::mutex> lock(mtx_);
    registered_user_ = user_at_domain;
}

bool SIPStateManager::is_registered() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return reg_state_ == RegistrationState::Registered;
}
