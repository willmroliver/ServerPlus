#ifndef INCLUDE_SERVER_H
#define INCLUDE_SERVER_H

#include <event-base.hpp>
#include <event.hpp>
#include <map>
#include <string>
#include "socket.hpp"
#include "thread-pool.hpp"
#include "handler.hpp"

using namespace libev;

namespace serv {

class Context;
class Handler;

class Server {
    private:
        int status = 0;
        std::string port;
        Socket listen_sock;
        EventBase base;
        ThreadPool thread_pool;
        std::unordered_map<evutil_socket_t, std::shared_ptr<Context>> ctx_pool;
        std::unordered_map<std::string, std::unique_ptr<Handler>> api;

        static event_callback_fn accept_callback;

    public:
        Server();
        Server(std::string port);
        Server(std::string port, unsigned thread_count);
        Server(Server &s) = delete;
        Server(Server &&s) = delete;
        ~Server() = default;

        /**
         * @brief Get the error status of the server. 0 indicates no error.
         * 
         * @return int 
         */
        inline int get_status() const {
            return status;
        };
        
        /**
         * @brief Assigns the handler callback to the path. Requests to the path will execute the callback given.
         * 
         * @param path The path to assign the callback to. Corresponds to the proto::Header 'path' field.
         * @param cb The callback to execute when this path is requested.
         */
        void set_endpoint(std::string path, HandlerFunc cb);

        /**
         * @brief If a callback has been assigned to the 'path' requested, exec_endpoints passes the context to the callback and executes; else returns false.
         * 
         * @param path The path to assign the callback to. Corresponds to the proto::Header 'path' field.
         * @param c The context of the current connection.
         * @return true The path exists in the API.
         * @return false The path does not exist.
         */
        bool exec_endpoint(std::string path, Context* c);

        /**
         * @brief Calls try_listen() and adds a persistent event to listen to & accept connections from the bound sock.
         * Then runs the event base loop. The exit status of the loop will be set on the Server.
        */
        void run();

        /**
         * @brief Called by accept_callback; attempts to accept an incoming connection attempt.
         */
        void accept_connection();

        /**
         * @brief Gracefully terminates the event base loop.
        */
        void stop();

        /**
         * @brief Get a pointer to the event bae.
         * 
         * @return const EventBase* const 
         */
        EventBase* const get_base();

        /**
         * @brief Pass any generic function to the thread pool, to later be executed by a thread, passing in the args given.
         * 
         * @tparam F The function type
         * @tparam Args The function arguments
         * @param f The function
         * @param args The arguments to execute the function with.
         */
        template <typename F, typename... Args>
        void allocate_work(F&& f, Args&& ...args) {
            thread_pool.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
        }
};

}

#endif