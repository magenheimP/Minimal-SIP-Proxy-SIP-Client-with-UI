#include "../include/proxy/metrics_server.hpp"
#include "../include/proxy/metrics_collector.hpp"
#include "../../networking/include/networking/tcp_socket.hpp"


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>

MetricsServer::MetricsServer() = default;

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
    TcpSocket server_socket;

    if (!server_socket.bind(port)) {
        perror("bind");
        return;
    }

    if (!server_socket.listen(10)) {
        perror("listen");
        return;
    }

    std::cout << "[Metrics] Server running on port " << port << std::endl;

    while (running) {
        std::string client_ip;
        uint16_t client_port;

        const int client_fd = server_socket.accept(client_ip, client_port);

        if (client_fd < 0) {
            continue;
        }

        // Read request (ignore content)
        char buffer[1024];
        server_socket.recvFromFd(client_fd, buffer, sizeof(buffer));

        // Prepare response
        std::string body = MetricsCollector::instance().to_string();

        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" +
            body;

        server_socket.sendToFd(client_fd, response);

        server_socket.closeClient(client_fd);
    }
}
