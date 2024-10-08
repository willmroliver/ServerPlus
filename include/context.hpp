#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

#include <event.hpp>
#include <crypt/exchange.hpp>
#include <string>
#include <thread>
#include <memory>
#include "secure-socket.hpp"
#include "header.pb.h"

using namespace libev;
using namespace crpt;

namespace serv {

class Server;

/**
 * @brief Encapsulates the state of an accepted connection, managing data reading and writing over arbitrarily many send & receive operations.
 */
class Context {
    private:
        Server* server;
        std::shared_ptr<SecureSocket> sock;
        std::shared_ptr<Event> event;
        std::shared_ptr<std::thread> thread;
        std::string header_data;
        std::string request_data;
        proto::Header header;
        static event_callback_fn receive_callback;
        static event_callback_fn handshake_callback;
        bool header_parsed = false;
        int fd = 0;

        /**
         * @brief Adds a new event to the server event base for this socket, passing itself as the arg.
         * 
         * @param what The kind of event to trigger on, e.g. EV_READ, EV_WRITE. See libevent
         * @param cb The callback to execute when the even triggers.
         */
        void new_event(short what, event_callback_fn cb);

        /**
         * @brief Adds a new receive event to the underlying socket; triggers the Context::receive_callback when data is available.
         */
        inline void new_read_event() {
            new_event(EV_READ|EV_PERSIST, receive_callback);
        }
        
        /**
         * @brief Adds a new handshake event to the underlying socket; triggers the Context::handshake_callback when data is available.
         */
        inline void new_handshake_event() {
            new_event(EV_READ, handshake_callback);
        }

        /**
         * @brief If a header has been parsed the complete request data received, processes the request
         */
        void handle_request();

        /**
         * @brief Resets the context, ready to receive the next request
         */
        void reset();

    public:
        Context(Server* server, SecureSocket&& sock);
        ~Context();

        /**
         * @brief Reads available data from the sock stream, then attempts to parse a header and request from the contents of the socket buffer
         */
        void read_sock();

        /**
         * @brief Sends data to the client via the open sock stream.
         * 
         * @param data Data to send to the client
         */
        bool send_message(const std::string& data);

        /**
         * @brief Logs and returns an error status to the peer
         * 
         * @param err_code See error-codes.hpp
         */
        void do_error(int err_code);

        inline const std::string get_header_data() const noexcept {
            return header_data;
        }

        inline const std::string get_request_data() const noexcept {
            return request_data;
        }

        void join() noexcept;
};

};

#endif