#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#include "context.hpp"
#include "server.hpp"

using namespace libev;
using namespace serv;

Context::Context(Server* s, evutil_socket_t fd, void (*cb)(Context* c)):
    server { s },
    fd { fd },
    cb { cb }
{
    sock_to_buffer = [] (char* dest, unsigned n, void* data) {
        auto context = (Context*)data;
        auto buffer = context->buffer;

        auto nbytes = recvfrom(context->fd, dest, n, 0, nullptr, 0);

        if (nbytes <= 0) {
            if (nbytes == -1) perror("context: recvfrom");
            else std::cout << "context: peer closed connection on sock " << context->fd << std::endl;
            context->end();

            return 0;
        }

        return (int)nbytes;
    };
}

Context::~Context() {
    end();
}

void Context::set_event(Event* e) {
    ev = e;
}

void Context::exec() {
    buffer.write(sock_to_buffer, buffer.bytes_free(), this);
    request = buffer.read_to(0);
    cb(this);
}

void Context::end() {
    server->remove(fd);
}

std::string Context::get_request() {
    return request;
}