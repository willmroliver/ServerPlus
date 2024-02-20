#ifndef INCLUDE_HANDLER_H
#define INCLUDE_HANDLER_H

#include <functional>
#include <string>

namespace serv {

class Server;
class Context;

using HandlerFunc = std::function<void(Server*, Context*)>;

class Handler {
    private:
        Server* s;
        std::string path;
        HandlerFunc cb;

    public:
        Handler(Server* s, std::string path, HandlerFunc cb);
        Handler(Handler& h) = default;
        Handler(Handler&& h) = default;
        ~Handler() = default;

        void exec(Context* c) const;
};  

}

#endif