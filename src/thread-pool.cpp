#include "thread-pool.hpp"
#include "logger.hpp"
#include "error-codes.hpp"

using namespace serv;

ThreadPool::ThreadPool():
    run { true },
    n { std::thread::hardware_concurrency() - 1 }
{
    if (!n) {
        n = 4;
    }

    start();
}

ThreadPool::ThreadPool(unsigned n):
    run { true },
    n { n }
{
    start();
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start() {
    for (auto i = 0; i < n; ++i) {
        std::promise<void> promise;
        thread_futures.push_back(promise.get_future());

        pool.emplace_back([this, i] (std::promise<void> p) {
            while (true) {
                try {
                    std::function<void()> task;

                    {
                        std::unique_lock lock { queue_mutex };

                        // Wait until there is work available or the thread pool has been halted.
                        condition.wait(lock, [this] () {
                            return !run || !queue.empty();
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
                catch (const std::exception& e) {
                    Logger::get().error(ERR_THREAD_POOL_THREAD_LOOP_ERROR, &e);
                }
            }    
        }, std::move(promise));
    }
}

void ThreadPool::stop(bool graceful) {
    try {
        if (run) {
            run = false;

            condition.notify_all();

            if (graceful) {
                for (const auto& f : thread_futures) {
                    f.wait();
                }
                for (auto& th : pool) {
                    th.join();
                }
            }
        }

        pool.clear();
    }
    catch (const std::exception& e) {
        Logger::get().error(ERR_THREAD_POOL_DESTROY_POOL_ERROR, &e);
    }   
}