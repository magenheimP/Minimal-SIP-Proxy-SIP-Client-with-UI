//
// Created by vladimirsubotic on 3/9/26.
//

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H


#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include "../../../common/include/common/thread_safe_queue.hpp"
using namespace std;

class ThreadPool
{
public:
    using Task = std::function<void()>;

private:

    vector<thread> workers;
    ThreadSafeQueue<Task> taskQueue;
    atomic<bool> running;

    void workerLoop();

public:

    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueue(Task&& task);

    void shutdown();

};


#endif //_THREAD_POOL_H