#ifndef INCLUDE_THREAD_H
#define INCLUDE_THREAD_H

#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

namespace serv {

class ThreadPool {
    private:
        std::vector<std::thread> pool;
        std::vector<std::future<void>> thread_futures;
        std::queue<std::function<void()>> queue;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool run;

    public:
        ThreadPool(unsigned n);
        ThreadPool(ThreadPool& pool) = delete;
        ThreadPool(ThreadPool&& pool) = delete;
        ~ThreadPool();

        /**
         * @brief Pass any generic function to the thread pool, to later be executed by a thread, passing in the args given.
         * 
         * @tparam F The function type
         * @tparam Args The function arguments
         * @param f The function
         * @param args The arguments to execute the function with.
         */
        template <typename F, typename... Args>
        void enqueue(F&& f, Args&& ...args) {
            if (!run) {
                return;
            }
            
            {
                // Lock the queue and add the new function.
                std::unique_lock lock { queue_mutex };
                queue.emplace([f = std::forward<F>(f), args = std::make_tuple(std::forward(args)...)] () mutable { 
                    std::apply(std::move(f), std::move(args));
                });
            }

            // Notify some waiting thread that there is available work in the queue.
            condition.notify_one();
        }

        /**
         * @brief Stops the thread pool. If graceful is set to true, this call blocks until all threads have finished executing.
         * 
         * @param graceful 
         */
        void stop(bool graceful = true);
};

}

#endif