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

#include "thread_pool.h"


TThreadPool::TThreadPool(size_t threadsCount) {
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

TThreadPool::~TThreadPool() {
    {
        std::unique_lock<std::mutex> lock(Mutex);
        IsDone = true;
    }
    Condition.notify_all();
    for(auto&& thread : Threads) {
        thread.join();
    }
}

