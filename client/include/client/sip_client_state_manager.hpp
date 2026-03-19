#pragma once
#include "common/logger.hpp"
#include <mutex>
#include <string>

class SIPClientStateManager {
public:
    enum class State {
        Idle,
        Registering,
        Registered,
        Calling,    // outgoing call in progress (caller side only)
        Ringing,    // 180 received (caller) OR incoming INVITE received (callee)
        InCall,
        Failed,
    };

    State       get_state()       const;
    bool        is_registered()   const;
    bool        is_calling()      const;
    bool        is_ringing()      const;
    bool        is_in_call()      const;
    bool        is_busy()         const;  // Calling || Ringing || InCall

    static std::string state_name(State s);

    std::string registered_user()   const;
    std::string active_call_id()    const;
    std::string active_remote_uri() const;

    void on_registering();
    void on_register_success(const std::string& username, const std::string& domain);
    void on_register_failed();

    // Outgoing call caller side, moves to Calling
    void on_calling(const std::string& call_id, const std::string& remote_uri);

    // Incoming call callee side, moves to Ringing (not Calling)
    void on_incoming_call(const std::string& call_id, const std::string& remote_uri);

    void on_ringing();
    void on_call_established();
    void on_call_terminated();

    void transition_to_unlocked(State s);
    void transition_to(State s);

private:


    mutable std::mutex  mtx_;
    State               state_          { State::Idle };
    std::string         registered_user_;
    std::string         active_call_id_;
    std::string         active_remote_uri_;
    common::Logger&     logger_         { common::Logger::instance() };
};