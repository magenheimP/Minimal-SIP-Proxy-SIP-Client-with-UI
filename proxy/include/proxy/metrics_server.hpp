#pragma once

#include <thread>
#include <atomic>
#include <cstdint>

class MetricsServer {
public:
    MetricsServer();
    ~MetricsServer();

    void start(uint16_t port);
    void stop();

private:
    void server_loop(uint16_t port) const;

    std::thread server_thread;
    std::atomic<bool> running{false};
};