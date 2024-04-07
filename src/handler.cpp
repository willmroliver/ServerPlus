#include "handler.hpp"

using namespace serv;

Handler::Handler(Server* s, std::string path, HandlerFunc cb):
    s { s },
    path { path },
    cb { cb }
{}

void Handler::exec(Context* c) const {
    cb(s, c);
}