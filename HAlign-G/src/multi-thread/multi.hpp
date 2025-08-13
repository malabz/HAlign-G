#pragma once
extern "C"
{
#include "../multiz/multiz.h"
}

#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>


class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads) : stop(false), busy(0) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        ++busy;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        --busy;
                        cv_finished.notify_one();
                    }
                }
                });
        }
    }

    template<typename Func, typename... Args>
    void execute(Func&& func, Args&&... args) {
        while (tasks.size() > workers.size())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.emplace(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        }
        condition.notify_one();
    }

    template<typename Func, typename... Args>
    void execute(void (Func::* func)(Args...), Func* obj, Args&&... args) {
        while (tasks.size() > workers.size())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.emplace(std::bind(std::mem_fn(func), obj, std::forward<Args>(args)...));
        }
        condition.notify_one();
    }


    void waitFinished() {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv_finished.wait(lock, [this] { return tasks.empty() && (busy == 0); });
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }
    std::mutex mutex_ns, mutex_fp; // Ìí¼Ó»¥³âËø
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable cv_finished;
    bool stop;
    size_t busy;
};

int mul_main_sv(int filenum, char* path);

int mul_main(int filenum, char* path);

extern ThreadPool* threadPool0;
extern ThreadPool* threadPool1;
extern ThreadPool* threadPool2;