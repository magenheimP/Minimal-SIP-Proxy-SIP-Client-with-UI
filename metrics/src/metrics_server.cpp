#include "../include/metrics_server.hpp"
#include "../include/metrics_coollector.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

MetricsServer::MetricsServer() {}

MetricsServer::~MetricsServer() {
    stop();
}

void MetricsServer::start(uint16_t port) {
    if (running) return;

    running = true;
    server_thread = std::thread(&MetricsServer::server_loop, this, port);
}

void MetricsServer::stop() {
    running = false;

    if (server_thread.joinable())
        server_thread.join();
}

void MetricsServer::server_loop(const uint16_t port) const {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    std::cout << "[Metrics] Server running on port " << port << std::endl;

    while (running) {
        const int client_fd = accept(server_fd, nullptr, nullptr);

        if (client_fd < 0) {
            continue;
        }

        // Read request (we don't really care about content)
        char buffer[1024];
        read(client_fd, buffer, sizeof(buffer));

        // Get metrics
        std::string body = MetricsCollector::instance().to_string();

        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" +
            body;

        send(client_fd, response.c_str(), response.size(), 0);

        close(client_fd);
    }

    close(server_fd);
}
