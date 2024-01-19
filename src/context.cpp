#include <iostream>
#include <thread>

#include "context.hpp"
#include "server.hpp"

using namespace libev;
using namespace serv;

/**
 * @brief The primary handler for incoming data events from accepted connections
 */
event_callback_fn Context::receive_callback = [] (evutil_socket_t fd, short flags, void* arg) {
    auto ctx = static_cast<Context*>(arg);
    if (ctx == nullptr) {
        return;
    }

    ctx->server->allocate_work([&ctx] () {
        ctx->handle_read_event();
        ctx->event.add();
    });
};

event_callback_fn Context::handshake_callback = [] (evutil_socket_t fd, short flags, void* arg) {
    auto ctx = static_cast<Context*>(arg);
    if (ctx == nullptr) {
        return;
    }

    if (ctx->sock.handshake_final()) {
        ctx->new_read_event();
        
        if (!ctx->event.add()) {
            ctx->server->get_base()->dump_status();
            return;
        }
    }
    else {
        // retry handshake until we succeed or peer closes connection
        ctx->sock.handshake_init();
    }
};

Context::Context(Server* server, std::shared_ptr<Socket>& s):
    server { server },
    sock { s },
    event { new_handshake_event() }
{
    if (!sock.handshake_init()) {
        // error - need to figure out a way to get the server to clean this context up after (probably set a status field)
        return;
    }

    if (!event.add()) {
        server->get_base()->dump_status();
        return;
    }
}

void Context::handle_request() {

}

void Context::reset() {
    header_data = "";
    header_parsed = false;
}

Event Context::new_event(short what, event_callback_fn cb) {
    return server->get_base()->new_event(sock.get_sock()->get_fd(), what, cb, this);
}

void Context::handle_read_event() {
    auto [nbytes, full] = sock.try_recv();

    if (nbytes <= 0) return;

    if (header_parsed) {
        request_data = sock.retrieve_message();

        if (request_data.size()) {
            handle_request();
            reset();
            return;
        }

        if (full) {
            // error
            sock.clear_buffer();
            reset();
            return;
        }
    }
    
    header_data = sock.retrieve_message();

    if (header_data.size()) {
        if (!header.ParseFromString(header_data)) {
            // error
            reset();
            return;
        }

        if (header.type() == "ping") {
            if (!sock.try_send(header_data)) {
                // error
            }

            reset();
            return;
        }

        header_parsed = true;
        return;        
    }
    
    if (full) {
        // error
        sock.clear_buffer();
        reset();
    }
}