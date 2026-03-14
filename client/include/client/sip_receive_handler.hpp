#pragma once
#include <string>
#include <mutex>
#include <condition_variable>
#include <algorithm>

class SIPReceiveHandler {
public:
    SIPReceiveHandler() = default;

    void handle_receive(const std::string& data);

    bool wait_for_response(int timeout_seconds);

    void reset();

    bool        get_register_received() const;
    std::string get_register_response() const;

    mutable std::mutex mtx;

private:
    std::condition_variable cv_;
    bool        register_response_received_ = false;
    std::string register_response_;
};
