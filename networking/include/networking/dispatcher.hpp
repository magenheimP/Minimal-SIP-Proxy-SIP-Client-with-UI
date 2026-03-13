//
// Created by lazarstani on 3/12/26.
//

#ifndef SIPTRAININGPROJECT_DISPATCHER_HPP
#define SIPTRAININGPROJECT_DISPATCHER_HPP
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include "../../../common/include/common/types.hpp"
#include "../../../common/include/common/thread_safe_queue.hpp"
#include "thread_pool.hpp"

class Dispatcher {
public:
    using Handler = std::function<void(const common::RawPacket&)>;

    Dispatcher(ThreadPool& pool, Handler handler);
    ~Dispatcher();

    void start();
    void stop();

    void push(common::RawPacket packet);

private:
    void loop();

    ThreadSafeQueue<common::RawPacket> queue;
    ThreadPool& threadPool;
    Handler handler;

    std::thread dispatcherThread;
    std::atomic<bool> running{false};
};

#endif //SIPTRAININGPROJECT_DISPATCHER_HPP