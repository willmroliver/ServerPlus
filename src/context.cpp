#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#include "context.hpp"
#include "server.hpp"

using namespace libev;
using namespace serv;

Context::Context(Server* server, Socket* sock):
    server { server },
    sock { sock }
{}

void Context::handle_request() {

}

void Context::reset() {
    header_data = "";
    header_parsed = false;
}

void Context::handle_read_event() {
    auto [nbytes, full] = sock->try_recv();

    if (nbytes <= 0) return;

    if (header_parsed) {
        request_data = sock->retrieve_data();

        if (request_data.size()) {
            handle_request();
            reset();
            return;
        }

        if (full) {
            // send error
            sock->clear_buffer();
            reset();
            return;
        }
    }
    
    header_data = sock->retrieve_data();

    if (header_data.size()) {
        if (!header.ParseFromString(header_data)) {
            // send error
            reset();
            return;
        }

        header_parsed = true;
        return;        
    }
    
    if (full) {
        // send error
        sock->clear_buffer();
        reset();
    }
}