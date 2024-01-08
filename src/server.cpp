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
    sockaddr_storage their_addr;
    socklen_t addrlen = sizeof their_addr;

    evutil_socket_t fd;

    if ((fd = accept(listener, (sockaddr*)&their_addr, &addrlen)) == -1) {
        perror("accept");
        return;
    }

    if (evutil_make_socket_nonblocking(fd) == -1) {
        perror("evutil_make_socket_nonblocking");
        return;
    }

    char host[NI_MAXHOST];
    int gai = getnameinfo((sockaddr*)&their_addr, addrlen, host, sizeof host, nullptr, 0, 0);

    if (!gai) std::cout << "server: connection accepted from " << host << " on sock " << fd << std::endl;
    else std::cerr << "getnameinfo: " << gai_strerror(gai) << "\n";

    Server* s = (Server*)arg;
    auto success = s->add(fd);

    if (!success) {
        s->remove(fd);
        std::cerr << "server: failed to add connection to event base on sock " << fd << std::endl;
    }
};

/**
 * @brief The primary handler for incoming data events from accepted connections
 */
event_callback_fn Server::receive_callback = [] (evutil_socket_t fd, short flags, void* arg) {
    auto context = (Context*)arg;
    
    if (context->read_sock()) {
        auto req = context->get_request();
        context->clear_request();
        // ... handle request
    }
};

Server::Server():
    port { "3993" }
{}

Server::Server(std::string port):
    port { port }
{}

int Server::try_listen() {
    addrinfo hints, *ai, *p;
    int gai;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((gai = getaddrinfo(nullptr, port.c_str(), &hints, &ai)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        exit(EXIT_FAILURE);
    }

    [[maybe_unused]]
    auto yes = 1;

    for (p = ai; p != nullptr; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind");
            close(listener);
            continue;
        }

        break;
    }

    if (p == nullptr) {
        fprintf(stderr, "server: failed to connect");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(ai);

    if (evutil_make_socket_nonblocking(listener) == -1) {
        perror("evutil_make_socket_nonblocking");
        exit(EXIT_FAILURE);
    }

    constexpr auto backlog = 10;

    if (listen(listener, backlog) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return listener;
}

void Server::run() {
    pool[listener] = base.new_event(try_listen(), EV_READ|EV_PERSIST, accept_callback, this);

    std::cout << "server: running on port " << port << ", listening on sock " << listener << std::endl;

    if (pool[listener]->add()) status = base.run();
    else status = -1;
}

void Server::stop() {
    if (pool[listener] != nullptr) {
        pool[listener]->remove();

        if (close(listener) == -1 && errno != EBADF) perror("close");
        listener = 0;

        std::cout << "server: closed listener" << std::endl;
    }

    if (base.loopexit()) status = 0;
    else status = -1;

    std::cout << "server: stopped with status " << status << std::endl;
}

int Server::get_listener_fd() const {
    return listener;
}

int Server::get_status() const {
    return status;
}

bool Server::add(evutil_socket_t fd) {
    auto context = new Context(this, fd);

    pool[fd] = base.new_event(fd, EV_READ|EV_PERSIST, receive_callback, context);

    context->set_event(pool[fd]);

    if (!pool[fd]->add()) {
        std::cerr << "server: failed to add event for sock " << fd << std::endl;
        remove(fd);
        return false;
    }

    return true;
}

void Server::remove(evutil_socket_t fd) {
    pool.erase(fd);
    close(fd);
}