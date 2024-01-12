#ifndef INCLUDE_SOCKET_H
#define INCLUDE_SOCKET_H

#include <event.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include "buffer.hpp"

using namespace libev;

namespace serv {

/**
 * @brief Manages a Berkeley socket. 
 * [This should probably have been encapsulated in its own wrapper library, but it works, and there's not much memory-management going on]
 */
class Socket {
    private: 
        evutil_socket_t fd;
        bool listening;
        sockaddr_storage addr;
        socklen_t addr_len;
        Buffer<1024> buf;
    
    public:
        Socket();
        Socket(Socket& sock) = default;
        Socket(Socket&& sock) = default;
        Socket& operator=(Socket& sock) = default;
        Socket& operator=(Socket&& sock) = default;
        ~Socket();

        inline const evutil_socket_t get_fd() const {
            return fd;
        }

        /**
         * @brief Attempts to listen for new connections on the specified port. 
         * See man listen & man getaddrinfo
         * 
         * @param port The port to listen to connections on
         * @param family Protocol family for socket
         * @param socktype Socket type
         * @param flags Input flags
         * @return bool The success or failure of the attempt to bind to the port. 
         */
        bool try_listen(std::string port, int family, int socktype, int flags);

        /**
         * @brief Attempts to listen for TCP IPv4 & IPv6 connections. 
         * See man listen & man getaddrinfo.
         * 
         * @param port The port to listen to connections on
         * @return bool The success or failure of the attempt to bind to the port. 
         */
        bool try_listen(std::string port);

        /**
         * @brief Attempts to accept a new connection. See man accept.
         * 
         * @return bool The success or failure of the call to accept().
         */
        bool try_accept(Socket& socket);

        /**
         * @brief Returns address host information, if the socket is managing an accepted connection.
         * See man getaddrinfo
         */
        std::string get_host() const;

        /**
         * @brief Attempts to close the socket. Even if an error occurs, the socket is guaranteed to be closed after the call.
         * See man close
         * 
         * @return bool Returns false if an error occurred.
         */
        bool close_fd();

        /**
         * @brief Attempts to read available data from the socket stream into a buffer.
         * See man recvfrom
         * 
         * @return std::pair<int, bool> The number of bytes read (-1 indicates an error) and whether the socket's buffer is full.
         */
        std::pair<int, bool> try_recv();
        
        /**
         * @brief Attempts to send the bytes stored in data over the network.
         * See man send
         * 
         * @return bool The success or failure of the attempt to send all the data. 
         */
        bool try_send(std::string data);

        /**
         * @brief Retrieves data from the buffer (FIFO) up to the first instance of delim.
         * 
         * @param delim A character (delimiting between blocks of data).
         * @return std::string The message, or an empty string if delim is not found.
         */
        std::string retrieve_data(char delim);

        /**
         * @brief Retrieves data from the buffer (FIFO) up to the first null-terminator.
         * 
         * @return std::string The message, or an empty string if a null-terminator is not found.
         */
        inline std::string retrieve_data() {
            return retrieve_data(0);
        };

        /**
         * @brief Clears the socket buffer.
         */
        void clear_buffer();
};

}

#endif