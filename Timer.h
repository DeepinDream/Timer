#ifndef TIMER_H_
#define TIMER_H_

#include<functional>
#include<chrono>
#include<thread>
#include<atomic>
#include<memory>
#include<mutex>
#include<condition_variable>

class InCycleRunner
{
public:
    InCycleRunner(): isFirstTime(true){start = std::chrono::system_clock::now();}
    template<typename _Rep, typename _Period>
    void Run(const std::chrono::duration<_Rep, _Period>& cycle_time, std::function<void()> task)
    {
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<_Rep, _Period>>(end - start);

        if(isFirstTime == true || duration >= cycle_time){
            //             std::cout << "get ServingCellPro per 10 seconds........." << std::endl;
            start = std::chrono::system_clock::now();
            isFirstTime = false;
            return task();
        }
    }

    void Clear()
    {
        isFirstTime = true;
    }

private:
    bool isFirstTime;
    std::chrono::time_point<std::chrono::system_clock> start;
};

class TimerKiller {
public:
    // returns false if killed:
    TimerKiller(): terminate(false){}
    template<class R, class P>
    bool wait_for( std::chrono::duration<R,P> const& time ) {
        std::unique_lock<std::mutex> lock(m);
        terminate = false;
        return !cv.wait_for(lock, time, [&]{return terminate;});
//        return !cv.wait_for(lock, time);
    }
    void kill() {
        std::unique_lock<std::mutex> lock(m);
        terminate=true; // should be modified inside mutex lock
        cv.notify_all(); // it is safe, and *sometimes* optimal, to do this outside the lock
    }
    // I like to explicitly delete/default special member functions:
    TimerKiller(TimerKiller&&)=delete;
    TimerKiller(TimerKiller const&)=delete;
    TimerKiller& operator=(TimerKiller&&)=delete;
    TimerKiller& operator=(TimerKiller const&)=delete;
private:
    mutable std::condition_variable cv;
    mutable std::mutex m;
    bool terminate;
};

class Timer
{
public:
    Timer() :expired_(true), try_to_expire_(false)
    {
    }

    Timer(const Timer& t) {
        expired_ = t.expired_.load();
        try_to_expire_ = t.try_to_expire_.load();
    }

    ~Timer() {
        Expire();
    }

    template<typename _Rep, typename _Period>
    void StartTimer(const std::chrono::duration<_Rep, _Period>& after, std::function<void()> task) {
        if (expired_ == false)
        {
            return;
        }

        expired_ = false;

        std::thread([this, after, task]() {
            while (!try_to_expire_) {
                task();
                killer_.wait_for(after);
            }

            {
                std::lock_guard<std::mutex> locker(mutex_);
                expired_ = true;
                expired_cond_.notify_one();
            }
        }).detach();
    }

    void Expire() {
        if (expired_ || try_to_expire_) {
            return;
        }

        killer_.kill();
        try_to_expire_ = true;
        {
            std::unique_lock<std::mutex> locker(mutex_);
            expired_cond_.wait(locker, [this] {return expired_ == true; });
            if (expired_ == true) {
                try_to_expire_ = false;
            }
        }
    }

    template<typename callable, class... arguments>
    void SyncWaitFor(int after, callable&& f, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
                (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
        std::this_thread::sleep_for(std::chrono::milliseconds(after));
        task();
    }

    template<typename Clock, typename Duration, typename callable, class... arguments>
    void SyncWaitUntil(const std::chrono::time_point<Clock,Duration>& sleep_time, callable&& f, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
                (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
        std::this_thread::sleep_until(sleep_time);
        task();
    }

    template<typename callable, class... arguments>
    void AsyncWait(int after, callable&& f, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task
                (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
        std::thread([after, task]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(after));
            task();
        }).detach();
    }

private:
    std::atomic<bool> expired_;
    std::atomic<bool> try_to_expire_;
    std::mutex mutex_;
    std::condition_variable expired_cond_;
    TimerKiller killer_;
};

#endif
