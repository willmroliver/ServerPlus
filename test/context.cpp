#include <boost/test/unit_test.hpp>
#include <string>
#include "context.hpp"
#include "server.hpp"
#include "client.hpp"
#include "error-codes.hpp"
#include "helpers.hpp"
#include "utility/time.hpp"
#include "header.pb.h"
#include "request.pb.h"
#include "error.pb.h"

struct ContextFixture {
    test::Client client;
    serv::Socket listener;
    libev::EventBase base;
    serv::SecureSocket sock;
    std::unique_ptr<serv::Context> ctx;

    ContextFixture(): 
        client { "8000" },
        listener {},
        base {},
        sock {}
    {
        clear_logger();
        listener.try_listen("8000", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
        client.try_connect();
        listener.try_accept(sock);

        sock.handshake_init();
        client.handshake_init();

        tiny_sleep();

        sock.handshake_final();
        client.handshake_final();
        
        ctx = std::make_unique<serv::Context>(nullptr, std::move(sock));
        tiny_sleep();
    }

    ~ContextFixture() {
        sock.close_fd();
        listener.close_fd();
        client.try_close();
    }
};

BOOST_FIXTURE_TEST_CASE( read_sock_parses_header, ContextFixture ) {
    serv::proto::Header header;
    header.set_timestamp(serv::util::sys_timestamp<std::chrono::microseconds>());
    header.set_path("/path/to/something");
    header.set_size(1);

    std::string header_data;
    header.SerializeToString(&header_data);

    client.try_send(header_data);

    tiny_sleep();
    ctx->read_sock();

    BOOST_ASSERT( ctx->get_header_data() == header_data );
}

BOOST_FIXTURE_TEST_CASE( read_sock_handles_malformed_data, ContextFixture ) {
    client.try_send("_@+$YOR(I_I£P@)");

    tiny_sleep();
    ctx->read_sock();

    ASSERT_ERR_LOGGED(ERR_CONTEXT_HANDLE_READ_FAILED);

    serv::proto::Error err;
    BOOST_ASSERT( err.ParseFromString(client.try_recv()) );
    BOOST_ASSERT( err.code() == ERR_CONTEXT_HANDLE_READ_FAILED );
}

struct ReadSockFixture : ContextFixture {
    serv::proto::Header header;
    serv::proto::Request request;
    std::string header_data;
    std::string request_data;

    ReadSockFixture(): 
        ContextFixture(),
        header {},
        request {}
    {
        header.set_timestamp(serv::util::sys_timestamp<std::chrono::microseconds>());
        header.set_path("/path/to/something");
        header.set_size(1);

        request.set_data("Hello, World!");

        header.SerializeToString(&header_data);
        request.SerializeToString(&request_data);
    }
};

BOOST_FIXTURE_TEST_CASE( read_sock_parses_header_and_request_together, ReadSockFixture ) {
    auto concat = [] (const std::string& a, const std::string& b) -> std::string {
        return a + '\0' + b;
    };

    client.try_send(concat(header_data, request_data));
    tiny_sleep();
    ctx->read_sock();

    BOOST_ASSERT( ctx->get_header_data() == header_data );
    BOOST_ASSERT( ctx->get_request_data() == request_data );
}

BOOST_FIXTURE_TEST_CASE( read_sock_parses_header_and_request_separately, ReadSockFixture ) {
    client.try_send(header_data);
    tiny_sleep();
    ctx->read_sock();
    BOOST_ASSERT( ctx->get_header_data() == header_data );

    client.try_send(request_data);
    tiny_sleep();
    ctx->read_sock();
    BOOST_ASSERT( ctx->get_request_data() == request_data );
}

BOOST_FIXTURE_TEST_CASE( read_sock_parses_header_and_request_sequentially, ReadSockFixture ) {
    auto concat = [] (const std::string& a, const std::string& b) -> std::string {
        return a + '\0' + b;
    };

    for (int i = 0; i < 10; i++) {
        client.try_send(concat(header_data, request_data));
        tiny_sleep();
        ctx->read_sock();

        BOOST_ASSERT( ctx->get_header_data() == header_data );
        BOOST_ASSERT( ctx->get_request_data() == request_data );
    }
}