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

#include "server.hpp"
#include "context.hpp"

using namespace libev;
using namespace serv;

/**
 * @brief The primary handler for incoming connection attempts
 */
event_callback_fn Server::accept_callback = [] (evutil_socket_t listener, short flags, void *arg) {
    Server* s = (Server*)arg;
    Socket* sock = nullptr;
    
    auto success = s->listen_sock.try_accept(sock);

    if (!success) {
        std::cerr << "server: failed to add connection to event base on sock " << sock->get_fd() << std::endl;
        return;
    }
};

/**
 * @brief The primary handler for incoming data events from accepted connections
 */
event_callback_fn Server::receive_callback = [] (evutil_socket_t fd, short flags, void* arg) {
    auto context = (Context*)arg;
    context->handle_read_event();
};

Server::Server():
    port { "3993" }
{}

Server::Server(std::string port):
    port { port }
{}

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

void Server::stop() {
    if (base.loopexit()) status = 0;
    else status = -1;

    std::cout << "server: stopped with status " << status << std::endl;
}

int Server::get_status() const {
    return status;
}

void Server::add_to_pool(Socket* sock) {
    pool[sock->get_fd()] = sock;
}
