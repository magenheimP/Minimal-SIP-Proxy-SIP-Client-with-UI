
#pragma once
#include <string>
#include <mutex>
#include <condition_variable>

class SIPReceiveHandler {
public:
    std::mutex mtx;
    std::condition_variable cv;
    bool register_response_received = false;
    std::string register_response;

    void handle_receive(const std::string& data) {
        std::string tmp = data;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper); // case-insensitive

        if ((tmp.find("SIP/2.0") != std::string::npos) &&
            ((tmp.find("200 OK") != std::string::npos) || (tmp.find("401") != std::string::npos))) {
            std::lock_guard<std::mutex> lock(mtx);
            register_response_received = true;
            register_response = data;

            cv.notify_one();
            }
    }

    bool wait_for_response(int timeout_seconds) {
        std::unique_lock<std::mutex> lock(mtx);
        return cv.wait_for(lock, std::chrono::seconds(timeout_seconds),
                           [this] { return register_response_received; });
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        register_response_received = false;
        register_response.clear();
    }
};