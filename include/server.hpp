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
        std::unordered_map<evutil_socket_t, Socket*> pool;
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
         * @brief Adds a connected socket to the connection pool.
         * 
         * @param sock The socket to add.
         */
        void add_to_pool(Socket* sock);
};

}

#endif