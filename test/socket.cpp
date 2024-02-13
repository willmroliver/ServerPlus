#include <boost/test/unit_test.hpp>
#include <string>
#include "socket.hpp"
#include "client.hpp"

struct TryListenTestCase {
    std::string port;
    int family;
    int socktype;
    int flags;
    bool expecting;
};

TryListenTestCase try_listen_tests[] = {
    { "", 0, 0, 0, false },
    { "8000", AF_UNSPEC, SOCK_STREAM, 0, true },
    { "8000", AF_INET6, SOCK_STREAM, 0, true },
    { "8000", AF_INET, SOCK_STREAM, 0, true },
    { "100", AF_UNSPEC, SOCK_STREAM, 0, false },
};

void do_try_listen_test(const TryListenTestCase& test) {
    serv::Socket sock;

    auto res = sock.try_listen(test.port, test.family, test.socktype, test.flags);
    BOOST_ASSERT( res == test.expecting );
}

BOOST_AUTO_TEST_CASE( socket_try_listen_table_test ) {
    for (const auto& test : try_listen_tests) {
        do_try_listen_test(test);
    }
}

struct F {
    serv::Socket listener;
    test::Client<1024> client;

    F(): client { "8000" } {
        listener.try_listen("8000", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
    }

    ~F() {
        listener.close_fd();
    }
};

BOOST_FIXTURE_TEST_CASE( socket_try_accept_test, F ) {
    BOOST_ASSERT( client.try_connect() );
    
    serv::Socket sock;

    BOOST_ASSERT( listener.try_accept(sock) );
    BOOST_ASSERT( sock.get_fd() > 0 );
    BOOST_ASSERT( sock.get_host() == "localhost" );
}