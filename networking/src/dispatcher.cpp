//
// Created by lazarstani on 3/12/26.
//

#include "../include/networking/dispatcher.hpp"

Dispatcher::Dispatcher(ThreadPool& pool, Handler handler)
    : threadPool(pool), handler(handler) {}

Dispatcher::~Dispatcher() {
    stop();
}

void Dispatcher::start() {
    if (running.load())
        return;

    running = true;

    dispatcherThread = std::thread(&Dispatcher::loop, this);
}

void Dispatcher::stop() {
    running = false;

    queue.stop();

    if (dispatcherThread.joinable())
        dispatcherThread.join();
}

void Dispatcher::push(common::RawPacket packet)
{
    queue.push(std::move(packet));
}

void Dispatcher::loop() {
    common::RawPacket packet;

    while (queue.pop(packet)) {

        threadPool.enqueue([this, packet]() {

            handler(packet);

        });

    }
}