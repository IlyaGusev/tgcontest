// Based on https://github.com/progschj/ThreadPool

#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class TThreadPool {
public:
    TThreadPool(size_t threadsCount=std::thread::hardware_concurrency());
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~TThreadPool();

private:
    // Need to keep track of threads so we can join them
    std::vector<std::thread> Threads;
    // The task queue
    std::queue<std::function<void()>> Tasks;

    // Synchronization
    std::mutex Mutex;
    std::condition_variable Condition;
    bool IsDone = false;
};

// The constructor just launches some amount of workers
inline TThreadPool::TThreadPool(size_t threadsCount) {
    for (size_t i = 0;i < threadsCount; ++i) {
        Threads.emplace_back(
            [this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(Mutex);
                        Condition.wait(lock, [this]{ return this->IsDone || !this->Tasks.empty(); });
                        if (IsDone && Tasks.empty()) {
                            return;
                        }
                        task = std::move(Tasks.front());
                        Tasks.pop();
                    }
                    task();
                }
            }
        );
    }
}

// Add new work item to the pool
template<class F, class... Args>
auto TThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(Mutex);

        // Don't allow enqueueing after stopping the pool
        if (IsDone) {
            throw std::runtime_error("enqueue on stopped TThreadPool");
        }

        Tasks.emplace([task](){ (*task)(); });
    }
    Condition.notify_one();
    return res;
}

// The destructor joins all threads
inline TThreadPool::~TThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(Mutex);
        IsDone = true;
    }
    Condition.notify_all();
    for(auto&& thread : Threads) {
        thread.join();
    }
}

