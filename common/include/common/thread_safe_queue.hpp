//
// Created by vladimirsubotic on 3/9/26.
//

#ifndef _THREAD_SAFE_QUEUE_HPP
#define _THREAD_SAFE_QUEUE_HPP

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
using namespace std;

template <typename T>
class ThreadSafeQueue
{
private:

    queue<T> queue_;
    mutex mutex_;
    condition_variable condition;
    bool stopped = false;

public:

    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    ThreadSafeQueue(ThreadSafeQueue const &) = delete;
    ThreadSafeQueue(ThreadSafeQueue &&) = delete;

    void push(const T &item)
    {
        lock_guard<mutex> lock(mutex_);
        queue_.push(item);
        condition.notify_one();
    }

    void push(T &&item)
    {
        lock_guard<mutex> lock(mutex_);
        queue_.push(move(item));
        condition.notify_one();
    }

    bool isEmpty()
    {
        lock_guard<mutex> lock(mutex_);
        return queue_.empty();
    }

    bool pop(T &item)
    {
        unique_lock<mutex> lock(mutex_);
        condition.wait(lock, [this]() { return !queue_.empty() || stopped; });

        if (queue_.empty() && stopped)
        {
            return false;
        }

        item = move(queue_.front());
        queue_.pop();
        return true;
    }

    void stop()
    {
        lock_guard<mutex> lock(mutex_);
        stopped = true;
        condition.notify_all();
    }
};

#endif //_THREAD_SAFE_QUEUE_HPP