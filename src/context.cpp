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
    auto ctx = (Context*)arg;

    auto execute_cb = [&ctx] () {
        ctx->handle_read_event();
        ctx->event.add();
    };

    auto thread = std::thread(execute_cb);
    thread.detach();
};

Context::Context(Server* server, std::shared_ptr<Socket>& sock):
    server { server },
    sock { sock },
    event { server->get_base()->new_event(sock->get_fd(), EV_READ, receive_callback, this) }
{
    if (!event.add()) {
        server->get_base()->dump_status();
    }
}

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

        if (header.type() == "ping") {
            if (!sock->try_send(header_data)) {
                // error
            }

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

const std::shared_ptr<Socket> Context::get_sock() const {
    return sock;
}