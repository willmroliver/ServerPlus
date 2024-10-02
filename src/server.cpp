#include "server.hpp"
#include "context.hpp"
#include "logger.hpp"
#include "error-codes.hpp"

using namespace libev;
using namespace serv;

/**
 * @brief The primary handler for incoming connection attempts
 */
event_callback_fn Server::accept_callback = [] (evutil_socket_t listener, short flags, void *arg) {
    ((Server*)arg)->accept_connection();
};

Server::Server():
    port { "3993" }
{}

Server::Server(std::string port):
    port { port }
{}

Server::~Server() {
    stop();
}

void Server::set_endpoint(std::string path, HandlerFunc cb) {
    api.emplace(path, std::make_unique<Handler>(this, path, cb));
}

bool Server::exec_endpoint(std::string path, Context* c) {
    if (api.find(path) == api.end()) {
        Logger::get().log("server: path not found");
        return false;
    }

    api[path]->exec(c);

    return true;
}

void Server::run() {
    if (!listen_sock.try_listen(port)) {
        status = -1;
        return;
    }

    Logger::get().log("server: running on port " + port);

    auto listen_event = base.new_event(listen_sock.get_fd(), EV_READ|EV_PERSIST, accept_callback, this);

    if (!listen_event.add()) {
        status = -1;
        return;
    }

    status = base.run();
}

void Server::accept_connection() {
    SecureSocket sock;

    if (!listen_sock.try_accept(sock)) {
        Logger::get().error(ERR_SERVER_ACCEPT_CONN_FAILED);
        Logger::get().error("server: accept_connection: failed on sock " + std::to_string(sock.get_fd()));
        return;
    }
    
    ctx_pool.emplace(sock.get_fd(), std::make_shared<Context>(this, std::move(sock)));
}

void Server::close_connection(evutil_socket_t fd) {
    thread_pool.enqueue([this, fd] () {
        auto it = ctx_pool.find(fd);
        if (it != ctx_pool.end()) {
            ctx_pool.erase(it);
        }
    });
}

void Server::stop() {
    for (const auto& [fd, ctx] : ctx_pool) {
        ctx->join();
    }
    
    base.loopexit();
    Logger::get().log("server: stopped with status " + std::to_string(status));
}

EventBase* const Server::get_base() {
    return &base;
}
