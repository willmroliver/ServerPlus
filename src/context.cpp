#include "context.hpp"

using namespace libev;
using namespace serv;

Context::Context(EventBase* base, Event* ev, void (*cb)(Context* c)):
    base { base },
    ev { ev },
    cb { cb }
{}