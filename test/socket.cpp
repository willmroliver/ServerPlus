#include <boost/test/unit_test.hpp>
#include <string>
#include "socket.hpp"

struct TryListenTestCase {
    std::string port;
    int family;
    int socktype;
    int flags;
    bool expecting;
};

TryListenTestCase try_listen_tests[] = {
    { "", 0, 0, 0, false }
};