#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

#include <event.hpp>
#include <crypt/exchange.hpp>
#include <string>
#include <memory>
#include "buffer.hpp"
#include "secure-socket.hpp"
#include "header.pb.h"

using namespace libev;
using namespace crpt;

namespace serv {

class Server;

class Context {
    private:
        Server* server;
        SecureSocket sock;
        Event event;

        std::string header_data;
        std::string request_data;
        proto::Header header;

        static event_callback_fn receive_callback;
        static event_callback_fn handshake_callback;

        /**
         * @brief When true, indicates a request header has been successfully parsed from the sock stream
         */
        bool header_parsed;

        /**
         * @brief If a header has been parsed the complete request data received, processes the request
         */
        void handle_request();

        /**
         * @brief Resets the context, ready to receive the next request
         */
        void reset();

        /**
         * @brief Adds a new read event to the underlying socket; triggers the Context::receive_callback when data is available.
         */
        inline void new_read_event() {
            event = new_event(EV_READ, receive_callback);
        }

        inline Event new_handshake_event() {
            return new_event(EV_READ, handshake_callback);
        }

    public:
        Context(Server* server, std::shared_ptr<Socket>& sock);
        
        /**
         * @brief Adds a new event to the server event base for this socket, passing itself as the arg.
         * 
         * @param what The kind of event to trigger on, e.g. EV_READ, EV_WRITE. See libevent
         * @param cb The callback to execute when the even triggers.
         */
        Event new_event(short what, event_callback_fn cb);

        /**
         * @brief Reads available data from the sock stream, then attempts to parse a header and request from the contents of the socket buffer
         */
        void handle_read_event();

};

};

#endif