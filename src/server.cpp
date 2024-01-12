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
    for (auto it = ctx_pool.begin(); it != ctx_pool.end(); ++it) {
        delete it->second;
    }
}

void Server::run() {
    if (!listen_sock.try_listen(port)) {
        status = -1;
        return;
    }

    std::cout << "server: running on port " << port << std::endl;

    auto listen_event = base.new_event(listen_sock.get_fd(), EV_READ|EV_PERSIST, accept_callback, this);

    if (!listen_event.add()) {
        status = -1;
        return;
    }

    status = base.run();
}

void Server::accept_connection() {
    auto sock = new Socket;

    if (!listen_sock.try_accept(sock)) {
        std::cerr << "server: failed to add connection to event base on sock " << sock->get_fd() << std::endl;
        return;
    }

    auto context = new Context(this, sock);

    ctx_pool[sock->get_fd()] = context;
}

void Server::stop() {
    if (base.loopexit()) status = 0;
    else status = -1;

    std::cout << "server: stopped with status " << status << std::endl;
}

EventBase* const Server::get_base() {
    return &base;
}

int Server::get_status() const {
    return status;
}
