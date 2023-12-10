#include "server.hpp"
#include <event2/event.h>

int main(int argc, char **argv) {
    db::Server server;

    server.run();

    return server.get_status() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}