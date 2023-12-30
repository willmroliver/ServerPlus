#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

#include <event-base.hpp>
#include <event.hpp>
#include <string>

#include "buffer.hpp"

using namespace libev;

namespace serv {

class Server;

class Context {
    private:
        Server* server;
        Event* ev;
        Buffer<512> buffer;
        std::string request;
        evutil_socket_t fd;

        void (*cb)(Context* c);
        int (*sock_to_buffer)(char* dest, unsigned n, void* data);

    public:
        Context(Server* s, evutil_socket_t fd, void (*cb)(Context*));
        Context(Context& c) = default;
        Context(Context&& c) = default;
        ~Context();

        void set_event(Event* e);

        /**
         * @brief Executes the Context callback.
         */
        void exec();

        /**
         * @brief Closes the context's connection and removes it from the server pool.
         */
        void end();

        /**
         * @brief Get the most recent request
         * 
         * @return std::string The latest request received
         */
        std::string get_request();
};

};

#endif