#include<iostream>

#include<string>

#include<memory>
#include <chrono>
#include "timer.h"

#include <ctime>

using namespace std;

void EchoFunc(std::string&& s) {

    std::cout << "test : " << s << endl;

}

void Add(int a, int b)
{
    std::cout << "a=" << a << " " << "b=" << b <<" "<<"a+b="<<a+b<< endl;
}

void TimerTest()
{
    Timer t;

    // run tasks periodically
    t.StartTimer(std::chrono::milliseconds(1000), std::bind(EchoFunc, "hello world!"));
    std::this_thread::sleep_for(std::chrono::seconds(4));
    std::cout << "try to expire timer!" << std::endl;
    t.Expire();


    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Async
    t.AsyncWait(2000, EchoFunc, "hello c++112!");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Sync
    t.SyncWaitFor(1000, EchoFunc, "hello world2!");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto now = std::chrono::system_clock::now();
    now += std::chrono::seconds(5);
    t.SyncWaitUntil(now, EchoFunc, "hello world3!");
}

void InCycleRunTest()
{
    using namespace std::chrono;
    auto start = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    auto duration = duration_cast<seconds>(now - start);
    InCycleRunner cycleRunner;
    while (duration.count() <= 10)
    {

        switch (duration.count()%2) {
        case 0:
            cycleRunner.Run(std::chrono::milliseconds(1000), std::bind(Add,1,3));
            break;
        case 1:
            cycleRunner.Run(std::chrono::milliseconds(1000), std::bind(Add,1,2));
            break;
        }

        now = std::chrono::system_clock::now();
        duration = duration_cast<seconds>(now - start);
        this_thread::sleep_for(milliseconds(100));
        cout << duration.count() << endl;
    }

    cout << duration.count() << endl;
}

int main()
{
    InCycleRunTest();
    TimerTest();

    return 0;


}
