#include <mutex>
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
    if (ctx == nullptr || ctx->server == nullptr) {
        return;
    }

    auto cpy = *ctx;

    ctx->server->allocate_work<std::function<void(Context)>, Context>([] (Context c) mutable {
        c.read_sock();
    }, std::move(cpy));
};

event_callback_fn Context::handshake_callback = [] (evutil_socket_t fd, short flags, void* arg) {
    auto ctx = static_cast<Context*>(arg);
    if (ctx == nullptr) {
        return;
    }

    if (ctx->sock->handshake_final()) {
        ctx->new_read_event();

        if (ctx->event->add()) {
            return;
        }

        if (ctx->server != nullptr) {
            ctx->server->get_base()->dump_status();
        }
    }
    else {
        Logger::get().log("server: handshake_final failed. retrying");
        ctx->sock->handshake_init();
    }
};

void Context::new_event(short what, event_callback_fn cb) {
    if (server != nullptr) {
        event = std::make_unique<Event>(server->get_base()->new_event(sock->get_fd(), what, cb, this));
    }
}

void Context::handle_request() {
    if (server == nullptr) {
        return;
    }

    if (!server->exec_endpoint(header.path(), this)) {
        do_error(ERR_CONTEXT_HANDLE_REQUEST_FAILED);
    }
}

void Context::reset() {
    request_data = "";
    header_data = "";
    header_parsed = false;
}

Context::Context(Server* server, SecureSocket&& s):
    server { server },
    sock { std::make_shared<SecureSocket>(std::move(s)) }
{
    fd = sock->get_fd();

    if (server == nullptr) {
        return;
    }

    new_handshake_event();

    if (!sock->handshake_init()) {
        Logger::get().error("server: handshake_init failed");
        return;
    }

    if (!event->add()) {
        server->get_base()->dump_status();
        return;
    }
}

void Context::read_sock() {
    auto [nbytes, can_write] = sock->try_recv();

    switch (nbytes) {
        case -2:
            Logger::get().log("server: context: secure-socket blocked try_recv(). attempting handshake");
            sock->handshake_init();
            return;
        case -1:
            do_error(ERR_CONTEXT_HANDLE_READ_FAILED);
            return;
        case 0:
            /** @todo implement some tidy-up, including removing context from Server::ctx_pool */
            return;
        default:
            break;
    }
    
    if (!header_parsed) {
        reset();
        
        header_data = sock->read_buffer();

        if (!header_data.size()) {
            if (!can_write) {
                do_error(ERR_CONTEXT_BUFFER_FULL);
                sock->clear_buffer();
            }

            return;
        }

        if (!header.ParseFromString(header_data)) {
            do_error(ERR_CONTEXT_HANDLE_READ_FAILED);
            Logger::get().error("server: context: protobuf: ParseFromString");
            return;
        }

        if (header.type() == proto::Header_Type::Header_Type_TYPE_PING) {
            if (!sock->try_send(header_data)) {
                do_error(ERR_CONTEXT_PING_FAILED);
                Logger::get().error("server: context: send ping failed");
            }
            
            return;
        }

        if (header.size() == 0) {
            handle_request();
            return;
        }

        header_parsed = true;
    }
    
    request_data = sock->read_buffer();
    
    if (request_data.size()) {
        handle_request();
        header_parsed = false;
    }
    
    if (!can_write) {
        do_error(ERR_CONTEXT_BUFFER_FULL);
        sock->clear_buffer();
        reset();
    }
}

bool Context::send_message(const std::string& data) {
    if (!sock->try_send(data)) {
        do_error(ERR_CONTEXT_SEND_MESSAGE_FAILED);
        return false;
    }

    return true;
}

void Context::do_error(int err_code) {
    auto &[ts, msg] = Logger::get().error(err_code);

    proto::Error err;
    err.set_code(err_code);
    err.set_message(msg);
    err.set_timestamp(ts);

    if (!sock->try_send(err.SerializeAsString())) {
        Logger::get().error(ERR_CONTEXT_DO_ERROR_FAILED);
    }
}