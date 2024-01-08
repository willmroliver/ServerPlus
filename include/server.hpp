#ifndef INCLUDE_SERVER_H
#define INCLUDE_SERVER_H

#include <event-base.hpp>
#include <event.hpp>
#include <map>
#include <string>
#include <memory>

using namespace libev;

namespace serv {

class Context;

class Server {
    private:
        std::string port;
        evutil_socket_t listener;
        EventBase base;
        std::unordered_map<evutil_socket_t, Event*> pool;
        int status = 0;

        static event_callback_fn accept_callback;
        static event_callback_fn receive_callback;

    public:
        Server();
        Server(std::string port);
        Server(Server &s) = delete;
        Server(Server &&s) = default;
        ~Server() = default;
 
        /**
         * @brief Attempts to bind to a socket to listen from.
         * 
         * @return int 0 on failure, or the socket file descriptor
         */
        int try_listen();

        /**
         * @brief Calls try_listen() and adds a persistent event to listen to & accept connections from the bound sock.
         * Then runs the event base loop. The exit status of the loop will be set on the Server.
        */
        void run();

        /**
         * @brief Gracefully terminates the event base loop.
        */
        void stop();
        
        /**
         * @brief Returns the socket file descriptor bound to by try_listen().
         * 
         * @return int The socket on which new connections are listened for.
         */
        int get_listener_fd() const;

        /**
         * @brief Get the error status of the server. 0 indicates no error.
         * 
         * @return int 
         */
        int get_status() const;
        
        /**
         * @brief Adds a read event listener to the server's event base for an accepted connection.
         * 
         * @param fd The socket on which the connection was accepted.
         * @return int 
         */
        bool add(evutil_socket_t fd);

        /**
         * @brief Closes a socket connection and removes it from the pool.
         * 
         * @param fd The socket to close.
         */
        void remove(evutil_socket_t fd);
};

}

#endif