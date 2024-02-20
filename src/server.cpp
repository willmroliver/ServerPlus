#include <string>
#include <iostream>
#include <utility>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <event.hpp>
#include <event-base.hpp>
#include <utility>
#include <iostream>
#include <thread>

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
    port { "3993" },
    thread_pool { 8 }
{}

Server::Server(std::string port):
    port { port },
    thread_pool { 8 }
{}

Server::Server(std::string port, unsigned thread_count):
    port { port },
    thread_pool { thread_count }
{}

void Server::set_endpoint(std::string path, HandlerFunc cb) {
    api[path] = std::make_unique<Handler>(this, path, cb);
}

bool Server::exec_endpoint(std::string path, Context* c) {
    if (api.find(path) == api.end()) {
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
    auto sock = std::make_shared<Socket>();

    if (!listen_sock.try_accept(*sock)) {
        Logger::get().error(ERR_SERVER_ACCEPT_CONN_FAILED);
        Logger::get().error("server: accept_connection: failed on sock " + std::to_string(sock->get_fd()));
        return;
    }

    ctx_pool[sock->get_fd()] = std::make_shared<Context>(this, sock);
}

void Server::stop() {
    thread_pool.stop();

    if (base.loopexit()) status = 0;
    else status = -1;

    Logger::get().log("server: stopped with status " + std::to_string(status));
}

EventBase* const Server::get_base() {
    return &base;
}
