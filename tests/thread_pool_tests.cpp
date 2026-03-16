//
// Created by vladimirsubotic on 3/13/26.
//

#include <iostream>
#include "../networking/include/networking/thread_pool.hpp"
#include <atomic>
#include <chrono>

using namespace std;

int main(){

    cout << "THREAD POOL TESTS"<<endl<<endl;

    const size_t num_threads = 4;
    const int num_tasks_test1 = 100;
    const int num_tasks_test2 = 1000;

    {
        cout<<"TEST1 BASIC TEST"<<endl;

        atomic<int> counter = 0;
        ThreadPool pool(num_threads);

        for(int i = 0; i < num_tasks_test1;i++){
            pool.enqueue([&counter]()
            {
                counter.fetch_add(1);
            });
        }

        pool.shutdown();

        if (counter.load() == num_tasks_test1)
        {
            cout<<"Test1 passed "<< counter.load()<<"/"<<num_tasks_test1<<endl<<endl;
        }
        else
        {
            cout<<"Test1 failed "<< counter.load()<<"/"<<num_tasks_test1<<endl<<endl;
        }
    }

    {
        cout<<"TEST2 SHUTDOWN WHILE BUSY TEST"<<endl;

        atomic<int> counter = 0;
        ThreadPool pool(num_threads);

        for (int i = 0; i < num_tasks_test2; i++)
        {
            pool.enqueue([&counter]() {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                counter.fetch_add(1);
            });
        }

        pool.shutdown();

        cout << "Test2 passed "<< counter.load() << "/"<<num_tasks_test2 << endl<<endl;
    }

    {
        cout<<"TEST3 ENQUEUE AFTER SHUTDOWN TEST"<<endl;

        atomic<int> counter = 0;
        ThreadPool pool(num_threads);
        pool.shutdown();

        pool.enqueue([&counter]() {
            counter.fetch_add(1);
        });

        if (counter.load() == 0)
        {
            cout<<"Test3 passed"<<endl;
        }
        else
        {
            cout<<"Test3 failed"<<endl;
        }
    }

    return 0;
}