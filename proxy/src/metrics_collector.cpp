#include "../include/proxy/metrics_collector.hpp"

MetricsCollector& MetricsCollector::instance() {
    static MetricsCollector instance;
    return instance;
}

// ---- Increment methods ----

void MetricsCollector::inc_messages_received() {
    ++messages_received;
}

void MetricsCollector::inc_messages_sent() {
    ++messages_sent;
}

void MetricsCollector::inc_active_calls() {
    ++active_calls;
}

void MetricsCollector::dec_active_calls() {
    if (active_calls > 0)
        --active_calls;
}

void MetricsCollector::inc_registered_users() {
    ++registered_users;
}

void MetricsCollector::dec_registered_users() {
    if (registered_users > 0)
        --registered_users;
}

// ---- Output ----

std::string MetricsCollector::to_string() const {
    std::string output;

    output += "SIP_messages_received counter\n";
    output += "SIP_messages_received " + std::to_string(messages_received.load()) + "\n";

    output += "SIP_messages_sent counter\n";
    output += "SIP_messages_sent " + std::to_string(messages_sent.load()) + "\n";

    output += "SIP_active_calls gauge\n";
    output += "SIP_active_calls " + std::to_string(active_calls.load()) + "\n";

    output += "SIP_registered_users gauge\n";
    output += "SIP_registered_users " + std::to_string(registered_users.load()) + "\n";

    return output;
}