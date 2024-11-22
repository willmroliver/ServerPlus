#ifndef INCLUDE_SOCKET_H
#define INCLUDE_SOCKET_H

#include <event.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include <cerrno>
#include <mutex>
#include "circular-buffer.hpp"

using namespace libev;

namespace serv {

/**
 * @brief Manages a Berkeley socket. 
 * [This should probably have been encapsulated in its own wrapper library, but it works, and there's not much memory-management going on]
 */
class Socket {
    protected:
        evutil_socket_t fd;
        bool listening;
        sockaddr_storage addr;
        socklen_t addr_len;
        CircularBuf buf;
        std::mutex recv_mux;
        std::mutex send_mux;
        std::mutex buf_mux;

    public:
        Socket();
        Socket(Socket& sock);
        Socket(Socket&& sock);
        Socket& operator=(Socket& sock);
        Socket& operator=(Socket&& sock);
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
        bool try_listen(const std::string& port, int family, int socktype, int flags);

        /**
         * @brief Attempts to listen for TCP IPv4 & IPv6 connections. 
         * See man listen & man getaddrinfo.
         * 
         * @param port The port to listen to connections on
         * @return bool The success or failure of the attempt to bind to the port. 
         */
        bool try_listen(const std::string& port);

        /**
         * @brief Attempts to connect to a listening socket via TCP with an IPv4 or IPv6 address.
         * See man connect & man getaddrinfo.
         * 
         * @param host IPv4 or IPv6 host address.
         * @param port Targeted port on host machine.
         * @return bool The success or failure of the connection attempt.
         */
        bool try_connect(const std::string& host, const std::string& port);

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
         * @param write_cb The callback to pass to the buffer's write function. See Buffer<T>::write()
         * @param arg The arg to pass in for access within the callback
         * @return std::pair<int, bool> The number of bytes read (-1 indicates an error) and whether the buffer has space remaining.
         */
        std::pair<uint32_t, bool> try_recv(uint32_t (*write_cb) (char* dest, uint32_t n, void* data) noexcept, void* arg);

        /**
         * @brief Attempts to read available data from the socket stream into a buffer. Uses a default zero-copy callback to write to the buffer
         * See man recvfrom
         * 
         * @return std::pair<int, bool> The number of bytes read (-1 indicates an error) and whether the buffer has space remaining.
         */
        std::pair<uint32_t, bool> try_recv();
        
        /**
         * @brief Attempts to send the bytes stored in data over the network.
         * See man send
         *
         * @param data The data to send 
         * @param terminate Whether to include the null-terminator, default is true.
         * @return bool The success or failure of the attempt to send all the data. 
         */
        bool try_send(const std::string& data, bool terminate=true);

        /**
         * @brief Attempts to send the bytes stored in data over the network.
         * See man send
         * 
         * @param data The data to send
         * @return bool The success or failure of the attempt to send all the data. 
         */
        bool try_send(const std::vector<char>& data);

        bool try_send(const std::vector<char>& data, ssize_t send(int, const void *, size_t, int));

        /**
         * @brief Retrieves data from the buffer (FIFO) up to the first instance of delim.
         * 
         * @param delim A character (delimiting between blocks of data).
         * @return std::string The message, or an empty string if delim is not found.
         */
        std::vector<char> read_buffer(char delim);

        /**
         * @brief Retrieves data from the buffer (FIFO) up to the first instance of delim.
         * 
         * @param delim A character sequence (delimiting between blocks of data).
         * @return std::string The message, or an empty string if delim is not found.
         */
        std::vector<char> read_buffer(std::string delim);

        /**
         * @brief Retrieves data from the buffer (FIFO) up to the first null-terminator.
         * 
         * @return std::string The message, or an empty string if a null-terminator is not found.
         */
        std::string read_buffer();

        /**
         * @brief Empties and returns the entire content of the buffer.
         * 
         * @return std::vector<char> 
         */
        std::vector<char> flush_buffer();

        /**
         * @brief Clears the socket buffer memory and resets internal state.
         */
        void clear_buffer();
};

}

#endif