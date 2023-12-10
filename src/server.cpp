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

using namespace libev;
using namespace db;

Server::Server():
    port { "3993" },
    base { EventBase() }
{}

Server::Server(std::string port):
    port { port },
    base { EventBase() }
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

    int yes;

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
    event_callback_fn accept_cb = [] (evutil_socket_t listener, short flags, void *arg) {
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
    };

    listen_event = base.new_event(try_listen(), EV_READ|EV_PERSIST, accept_cb);

    std::cout << "server: running on port " << port << ", listening on sock " << listener << std::endl;
    
    if (listen_event->add()) status = base.run();
    else status = -1;
}

void Server::stop() {
    if (listen_event != nullptr) {
        listen_event->remove();

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