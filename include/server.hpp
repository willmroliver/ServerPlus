#ifndef INCLUDE_SERVER_H
#define INCLUDE_SERVER_H

#include <event-base.hpp>
#include <event.hpp>
#include <map>
#include <string>
#include <memory>
#include "socket.hpp"

using namespace libev;

namespace serv {

class Context;

class Server {
    private:
        std::string port;
        Socket listen_sock;
        EventBase base;
        std::unordered_map<evutil_socket_t, Context*> ctx_pool;
        int status = 0;

        static event_callback_fn accept_callback;

    public:
        Server();
        Server(std::string port);
        Server(Server &s) = delete;
        Server(Server &&s) = default;
        ~Server();

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
         * @brief Get the error status of the server. 0 indicates no error.
         * 
         * @return int 
         */
        int get_status() const;
};

}

#endif