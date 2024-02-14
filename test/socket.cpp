#include <boost/test/unit_test.hpp>
#include <string>
#include "socket.hpp"
#include "client.hpp"
#include "error-codes.hpp"
#include "helpers.hpp"

struct TryListenTestCase {
    std::string port;
    int family;
    int socktype;
    int flags;
    bool expecting;
    std::vector<int> errors;
};

std::vector<TryListenTestCase> try_listen_tests {
    { "", 0, 0, 0, false, { ERR_SOCKET_GET_ADDR_INFO_FAILED } },
    { "100", AF_UNSPEC, SOCK_STREAM, 0, false, { ERR_SOCKET_BIND_SOCKET_FAILED } },
    { "8000", AF_UNSPEC, SOCK_STREAM, 0, true, {} },
    { "8000", AF_INET6, SOCK_STREAM, 0, true, {} },
    { "8000", AF_INET, SOCK_STREAM, 0, true, {} },
};

void do_try_listen_test(const TryListenTestCase& test) {
    init_test();

    serv::Socket sock;

    auto res = sock.try_listen(test.port, test.family, test.socktype, test.flags);
    BOOST_ASSERT( res == test.expecting );
    ASSERT_ERR_LOGGED( test.errors );
}

BOOST_AUTO_TEST_CASE( socket_try_listen_table_test ) {
    RUN_TEST_CASES<TryListenTestCase>( do_try_listen_test, try_listen_tests );
}

struct AcceptFixture {
    serv::Socket listener;
    test::Client<1024> client;

    AcceptFixture(): client { "8000" } {
        init_test();
        listener.try_listen("8000", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
    }

    ~AcceptFixture() {
        client.try_close();
        listener.close_fd();
    }
};

BOOST_FIXTURE_TEST_CASE( socket_try_accept_test, AcceptFixture ) {
    BOOST_ASSERT( client.try_connect() );
    
    serv::Socket sock;

    BOOST_ASSERT( listener.try_accept(sock) );
    BOOST_ASSERT( sock.get_fd() > 0 );
    BOOST_ASSERT( sock.get_host() == "localhost" );
}

struct SendFixture {
    serv::Socket listener;
    serv::Socket sender;
    test::Client<1024> client;

    SendFixture(): client { "8000" } {
        init_test();
        listener.try_listen("8000", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
        client.try_connect();
        listener.try_accept(sender);
    }

    ~SendFixture() {
        sender.close_fd();
        client.try_close();
        listener.close_fd();
    }
};

struct TrySendTestCase {
    std::vector<std::string> data;
};

std::vector<TrySendTestCase> tests {
    { { "" } },
    { { "0123456789" } },
    { { "", "98HEF24tqgo[']", "§12!@£$%^&*()   \n\n QEFGQvd\r\r" } },
    { { "\0\0\0" } },
};

void do_try_send_test(const TrySendTestCase& test, serv::Socket& sock) {
    for (const auto& s : test.data) {
        BOOST_ASSERT( sock.try_send(s) );
    }
}

BOOST_FIXTURE_TEST_CASE( socket_try_send_table_test, SendFixture ) {
    RUN_TEST_CASES<TrySendTestCase, serv::Socket>( do_try_send_test, tests, sender );
}

BOOST_AUTO_TEST_CASE( socket_try_send_fails_test ) {
    init_test();
    serv::Socket sock;

    BOOST_ASSERT( !sock.try_send("123456789") );
    ASSERT_ERR_LOGGED( ERR_SOCKET_INVALID_SEND_ATTEMPT );

    serv::Socket listener;
    test::Client<1024> client { "8000" };
    listener.try_listen("8000");
    client.try_connect();
    listener.try_accept(sock);

    auto mock_send = [] (int i, const void* j, size_t k, int l) -> ssize_t { 
        return -1; 
    };

    BOOST_ASSERT( !sock.try_send(std::vector<char>('0'), mock_send) );
    ASSERT_ERR_LOGGED( ERR_SOCKET_SEND_FAILED );
}