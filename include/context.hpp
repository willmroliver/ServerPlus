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

    public:
        Context(Server* server, std::shared_ptr<Socket>& sock);

        /**
         * @brief Reads available data from the sock stream, then attempts to parse a header and request from the contents of the socket buffer
         */
        void handle_read_event();
};

};

#endif