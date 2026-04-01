#pragma once

#include <atomic>
#include <string>
#include <cstdint>

class MetricsCollector {
public:
    static MetricsCollector& instance();

    void inc_messages_received();
    void inc_messages_sent();

    void inc_active_calls();
    void dec_active_calls();

    void inc_registered_users();
    void dec_registered_users();

    std::string to_string() const;

private:
    MetricsCollector() = default;

    std::atomic<int64_t> messages_received{0};
    std::atomic<int64_t> messages_sent{0};
    std::atomic<int64_t> active_calls{0};
    std::atomic<int64_t> registered_users{0};
};