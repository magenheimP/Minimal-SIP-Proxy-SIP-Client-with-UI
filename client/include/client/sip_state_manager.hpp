#pragma once
#include <string>
#include <mutex>


class SIPStateManager {
public:
    enum class RegistrationState {
        Idle,
        InProgress,
        Registered,
        Failed
    };

    SIPStateManager() = default;

    RegistrationState get_registration_state() const;
    void set_registration_state(RegistrationState s);

    std::string registered_user() const;
    void set_registered_user(const std::string& user_at_domain);

    bool is_registered() const;

private:
    mutable std::mutex    mtx_;
    RegistrationState     reg_state_   = RegistrationState::Idle;
    std::string           registered_user_;
};
