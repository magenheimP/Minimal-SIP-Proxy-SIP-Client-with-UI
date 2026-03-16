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

    bool is_in_call()  const;
    bool is_ringing()  const;
    bool is_calling()  const;

    std::string active_call_id()    const;
    std::string active_remote_uri() const;

    void on_calling(const std::string& call_id,
                    const std::string& remote_uri);
    void on_ringing();
    void on_call_established();
    // State:Registered (back to registered)
    void on_call_terminated();
private:
    void transition_to(State s);

    mutable std::mutex mtx_;
    State state_ = State::Idle;
    std::string registered_user_;
    std::string active_call_id_;
    std::string active_remote_uri_;
};