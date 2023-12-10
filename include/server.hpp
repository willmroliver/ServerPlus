#ifndef INCLUDE_SERVER_H
#define INCLUDE_SERVER_H

#include <event-base.hpp>
#include <event.hpp>
#include <map>
#include <string>
#include <memory>

using namespace libev;

namespace db {

class Server {
    private:
        std::string port;
        evutil_socket_t listener;
        EventBase base;
        Event* listen_event;
        int status = 0;

    public:
        Server();
        Server(std::string port);
        Server(Server &s) = delete;
        Server(Server &&s) = default;
        ~Server() = default;
 
        /**
         * Attempts to bind to a socket to listen from.
         * 
         * \return 0 on failure, or the socket file descriptor
         */
        int try_listen();

        /**
         * Adds a handler to accept connection requests from the socket bound to by try_listen().
         * Then runs the event base loop. The exit status of the loop will be set on the Server.
        */
        void run();

        /**
         * Gracefully terminates the event base loop.
        */
        void stop();

        /**
         * Returns the socket file descriptor bound to by try_listen().
         * 
         * \return The socket on which new connections are listened for.
        */
        int get_listener_fd() const;

        int get_status() const;
};

}

#endif