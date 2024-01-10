#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

#include <event.hpp>
#include <exchange.hpp>
#include <string>
#include "buffer.hpp"
#include "header.pb.h"

using namespace libev;
using namespace crpt;

namespace serv {

class Server;
class Socket;

class Context {
    private:
        Server* server;
        Socket* sock;
        std::string header_data;
        std::string request_data;
        proto::Header header;

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
        Context(Server* server, Socket* sock);
        Context(Context& c) = default;
        Context(Context&& c) = default;
        ~Context();

        /**
         * @brief Reads available data from the sock stream, then attempts to parse a header and request from the contents of the socket buffer
         */
        void handle_read_event();
};

};

#endif