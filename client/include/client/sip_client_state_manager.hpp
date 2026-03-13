#pragma once
#include <string>
#include <mutex>

class SIPClientStateManager {
public:
    enum class State {
        Idle,
        Registering,
        Registered,
        Calling,
        Ringing,
        InCall,
        Failed
    };

    SIPClientStateManager() = default;

    State get_state() const;

    std::string registered_user() const;

    bool is_registered() const;

    void on_registering();
    void on_register_success(const std::string& username, const std::string& domain);
    void on_register_failed();
    void transition_to_unlocked(State s);

private:
    void transition_to(State s);

    mutable std::mutex mtx_;
    State              state_           = State::Idle;
    std::string        registered_user_;
};