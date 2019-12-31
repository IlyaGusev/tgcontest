// Based on https://github.com/progschj/ThreadPool, altered to the project code style.
//
// Copyright (c) 2012 Jakob Progsch, Václav Zeman
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.

// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:

// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.

// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
// distribution.

#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

class TThreadPool {
public:
    // The constructor just launches some amount of workers
    TThreadPool(size_t threadsCount=std::thread::hardware_concurrency());

    // Add new work item to the pool
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // The destructor joins all threads
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

