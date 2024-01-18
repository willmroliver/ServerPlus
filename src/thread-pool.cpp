#include "thread-pool.hpp"
#include <iostream>

using namespace serv;

ThreadPool::ThreadPool(unsigned n):
    run { true } 
{
    for (auto i = 0; i < n; ++i) {
        std::promise<void> promise;
        thread_futures.push_back(promise.get_future());

        pool.emplace_back([this] (std::promise<void> p) {
            while (true) {
                std::function<void()> task;

                {
                    // Wait until there is work available or the thread pool has been halted.
                    std::unique_lock lock { queue_mutex };
                    condition.wait(lock, [this] () {
                        return !(run && queue.empty());
                    });

                    if (!run && queue.empty()) {
                        p.set_value();
                        return;
                    }

                    task = queue.front();
                    queue.pop();
                }

                task();
            }
        }, std::move(promise));
    }
}

ThreadPool::~ThreadPool() {
    if (run) {
        stop();
    }

    for (auto& th : pool) {
        th.join();
    }
}

void ThreadPool::stop(bool graceful) {
    run = false;
    condition.notify_all();
    
    if (graceful) {
        for (const auto& future : thread_futures) {
            future.wait();
        }
    }
}