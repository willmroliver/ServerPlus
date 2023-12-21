#ifndef INCLUDE_CONTEXT_H
#define INCLUDE_CONTEXT_H

#include <event-base.hpp>
#include <event.hpp>
#include <string>

using namespace libev;

namespace serv {

class Context {
    private:
        EventBase* base;
        Event* ev;
        void (*cb)(Context* c);
        std::string request;

    public:
        Context(EventBase* base, Event* ev, void (*cb)(Context* c));
        Context(Context& c) = default;
        Context(Context&& c) = default;
        ~Context() = default;
};

};

#endif