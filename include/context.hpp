#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

#include <event-base.hpp>
#include <event.hpp>
#include <exchange.hpp>
#include <string>
#include "buffer.hpp"

using namespace libev;
using namespace crpt;

namespace serv {

class Server;

class Context {
    private:
        Server* server;
        Event* ev;
        Buffer<512> buffer;

        std::string request;
        evutil_socket_t fd;

        int (*sock_to_buffer)(char* dest, unsigned n, void* data);

    public:
        Context(Server* s, evutil_socket_t fd);
        Context(Context& c) = default;
        Context(Context&& c) = default;
        ~Context();

        void set_event(Event* e);

        /**
         * @brief Reads available data from the socket into the Context buffer. 
         * If a full request is available in the buffer, the data is read from the buffer into the Context request.
         * 
         * @return bool Whether or not an entire request has been read.
         */
        bool read_sock();

        /**
         * @brief Closes the context's connection and removes it from the server pool.
         */
        void end();

        /**
         * @brief Get the current request data
         * 
         * @return std::string The latest request received
         */
        std::string get_request();

        /**
         * @brief Clear the current request data
         */
        void clear_request();
};

};

#endif