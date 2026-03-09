//
// Created by vladimirsubotic on 3/9/26.
//

#include "thread_pool.h"

ThreadPool::ThreadPool(size_t numThreads) : running(true)
{
	for(size_t  i = 0; i < numThreads; i++)
	{
	    workers.emplace_back([this]() { this->workerLoop(); });
	}
}

ThreadPool::~ThreadPool()
{
	shutdown();
}

void ThreadPool::workerLoop()
{
 	 Task task;
	 while (running.load() && taskQueue.pop(task)) {
     	task();
    }
}

void ThreadPool::enqueue(Task&& task)
{
	if (running.load()) {
        taskQueue.push(move(task));
    }
}

void ThreadPool::shutdown()
{
	running.store(false);
    taskQueue.stop();
    for (auto &t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
    workers.clear();
}
