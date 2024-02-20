#include <iostream>
#include <thread>

#include "context.hpp"
#include "server.hpp"
#include "logger.hpp"
#include "error-codes.hpp"
#include "error.pb.h"

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
        Logger::get().log("server: handshake_final failed. retrying");
        ctx->sock.handshake_init();
    }
};

Context::Context(Server* server, std::shared_ptr<Socket>& s):
    server { server },
    sock { s },
    event { new_handshake_event() }
{
    if (!sock.handshake_init()) {
        Logger::get().error("server: handshake_init failed");
        return;
    }

    if (!event.add()) {
        server->get_base()->dump_status();
        return;
    }
}

void Context::handle_request() {
    if (!server->exec_endpoint())
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

    switch (nbytes) {
        case -2:
            Logger::get().log("server: context: secure-socket blocked try_recv(). attempting handshake");
            sock.handshake_init();
            return;
        case -1:
            do_error(ERR_CONTEXT_HANDLE_READ_FAILED);
            return;
        case 0:
            Logger::get().log("server: context: connection closed");
            return;
        default:
            break;
    }

    if (header_parsed) {
        request_data = sock.read_buffer();
        
        if (request_data.size()) {
            handle_request();
            reset();
            return;
        }

        if (full) {
            do_error(ERR_CONTEXT_BUFFER_FULL);
            sock.clear_buffer();
            reset();
            return;
        }
    }
    
    header_data = sock.read_buffer();

    if (header_data.size()) {
        if (!header.ParseFromString(header_data)) {
            do_error(ERR_CONTEXT_HANDLE_READ_FAILED);
            Logger::get().error("server: context: protobuf: ParseFromString");
            reset();
            return;
        }

        if (header.type() == "ping") {
            if (!sock.try_send(header_data)) {
                do_error(ERR_CONTEXT_PING_FAILED);
                Logger::get().error("server: context: send ping failed");
            }

            reset();
            return;
        }

        header_parsed = true;
        return;        
    }
    
    if (full) {
        do_error(ERR_CONTEXT_BUFFER_FULL);
        sock.clear_buffer();
        reset();
    }
}

void Context::do_error(int err_code) {
    auto &[ts, msg] = Logger::get().error(err_code);

    proto::Error err;
    err.set_code(err_code);
    err.set_message(msg);
    err.set_timestamp(ts);

    if (!sock.try_send(err.SerializeAsString())) {
        Logger::get().error(ERR_CONTEXT_DO_ERROR_FAILED);
    }
}