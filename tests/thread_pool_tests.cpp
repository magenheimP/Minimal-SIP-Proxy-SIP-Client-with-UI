//
// Created by vladimirsubotic on 3/13/26.
//

#include <iostream>
#include "../networking/include/networking/thread_pool.hpp"
#include <atomic>
#include <chrono>

int main(){

    cout << "THREAD POOL TESTS"<<endl<<endl;

    {
        cout<<"TEST1 BASIC TEST"<<endl;

        atomic<int> counter = 0;
        ThreadPool pool(4);

        for(int i = 0; i < 100;i++){
            pool.enqueue([&counter]()
            {
                counter.fetch_add(1);
            });
        }

        pool.shutdown();

        if (counter.load() == 100)
        {
            cout<<"Test1 passed "<< counter.load()<<"/100"<<endl<<endl;
        }
        else
        {
            cout<<"Test1 failed "<< counter.load()<<"/100"<<endl<<endl;
        }
    }

    {
        cout<<"TEST2 SHUTDOWN WHILE BUSY TEST"<<endl;

        atomic<int> counter = 0;
        ThreadPool pool(4);

        for (int i = 0; i < 1000; i++)
        {
            pool.enqueue([&counter]() {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                counter.fetch_add(1);
            });
        }

        pool.shutdown();

        cout << "Test2 passed "<< counter.load() << "/1000" << endl<<endl;
    }

    {
        cout<<"TEST3 ENQUEUE AFTER SHUTDOWN TEST"<<endl;

        atomic<int> counter = 0;
        ThreadPool pool(4);
        pool.shutdown();

        pool.enqueue([&counter]() {
            counter.fetch_add(1);
        });

        if (counter.load() == 0)
        {
            cout<<"Test3 passsed"<<endl;
        }
        else
        {
            cout<<"Test3 failed"<<endl;
        }
    }

    return 0;
}