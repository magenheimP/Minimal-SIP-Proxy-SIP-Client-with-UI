#include "client/sip_receive_handler.hpp"

#include <chrono>
#include <algorithm>

void SIPReceiveHandler::handle_receive(const std::string& data)
{

    std::string upper = data;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    const bool is_sip_response = upper.find("SIP/2.0") != std::string::npos;
    const bool is_register_reply =
        upper.find("200 OK") != std::string::npos ||
        upper.find("401")    != std::string::npos;

    if (is_sip_response && is_register_reply) {
        std::lock_guard<std::mutex> lock(mtx);
        register_response_received_ = true;
        register_response_ = data;
        cv_.notify_one();
    }
}

bool SIPReceiveHandler::wait_for_response(int timeout_seconds)
{
    std::unique_lock<std::mutex> lock(mtx);
    return cv_.wait_for(lock,
                        std::chrono::seconds(timeout_seconds),
                        [this] { return register_response_received_; });
}

void SIPReceiveHandler::reset()
{
    std::lock_guard<std::mutex> lock(mtx);
    register_response_received_ = false;
    register_response_.clear();
}

bool SIPReceiveHandler::get_register_received() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return register_response_received_;
}

std::string SIPReceiveHandler::get_register_response() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return register_response_;
}
