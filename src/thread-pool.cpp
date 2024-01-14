#include "thread-pool.hpp"

using namespace serv;

ThreadPool::ThreadPool(unsigned n):
    run { true } 
{
    for (auto i = 0; i < n; ++i) {
        std::promise<void> promise;
        thread_futures.emplace(promise.get_future());

        pool.emplace(std::thread([=] (std::promise<void> p) {
            while (true) {
                std::function<void()> task;

                {
                    // Wait until there is work available or the thread pool has been halted.
                    std::unique_lock lock { queue_mutex };
                    condition.wait(queue_mutex, [this] () {
                        return !(run && queue.empty());
                    })

                    if (!run && queue.empty()) {
                        p.set_value();
                        return;
                    }

                    task = queue.front();
                    queue.pop();
                }

                task();
            }
        }, std::move(promise)));
    }
}

ThreadPool::~ThreadPool() {
    run = false;
}

void ThreadPool::stop(bool graceful = true) {
    run = false;
    for (const auto &f : thread_futures) {
        f.wait();
    }
}