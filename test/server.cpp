#include <thread>
#include <future>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <crypt/exchange.hpp>
#include "utility/time.hpp"
#include "client.hpp"
#include "server.hpp"
#include "context.hpp"
#include "logger.hpp"
#include "handler.hpp"
#include "helpers.hpp"
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"
#include "header.pb.h"

struct ServerFixture {
    test::Client client;
    serv::Server s;
    std::thread t;

    ServerFixture(): client { "8000" }, s { "8000" } {
        auto run = [=] () {
            s.run();
        }; 
        
        t = std::thread(run);
        t.detach();
    }

    ~ServerFixture() {
        s.stop();
    }
};

BOOST_FIXTURE_TEST_CASE( handshake_integration_test, ServerFixture ) {
    BOOST_ASSERT(client.try_connect());

    serv::proto::HostHandshake host_hs;

    BOOST_ASSERT(host_hs.ParseFromString(client.try_recv()));
    auto host_pk_str = host_hs.public_key();

    crpt::Exchange dh { "ffdhe2048" };
    crpt::PublicKeyDER host_pk;

    host_pk.from_vector({ host_pk_str.begin(), host_pk_str.end() });
    serv::proto::PeerHandshake peer_hs;

    auto peer_pk = dh.get_public_key().to_vector();
    peer_hs.set_public_key({ peer_pk.begin(), peer_pk.end() });

    BOOST_ASSERT(client.try_send(peer_hs.SerializeAsString()));

    auto res = client.try_recv();
    BOOST_ASSERT(res.size() == 1 && res[0] == 1);

    BOOST_ASSERT(client.try_close());
}

BOOST_FIXTURE_TEST_CASE( ping_integration_test, ServerFixture ) {
    client.try_connect();
    client.handshake_init();

    BOOST_ASSERT(client.handshake_final());

    using namespace serv::proto;
    auto ping_ts = serv::util::sys_timestamp<std::chrono::microseconds>;

    Header ping_header;
    ping_header.set_type(Header_Type::Header_Type_TYPE_PING);
    ping_header.set_size(0);
    ping_header.set_timestamp(ping_ts());
    BOOST_ASSERT(client.try_send(ping_header.SerializeAsString()));

    auto res = client.try_recv();
    BOOST_ASSERT(ping_header.ParseFromString(res));

    serv::Logger::get().log("PING: " + std::to_string(ping_ts() - ping_header.timestamp()));
}

BOOST_FIXTURE_TEST_CASE( handler_integration_test, ServerFixture ) {
    const std::string PATH_1 = "/test/1";
    const std::string MESSAGE_1 = "You've made it!";

    s.set_endpoint(PATH_1, [&MESSAGE_1] (serv::Server* srv, serv::Context* ctx) {  
        ctx->send_message(MESSAGE_1);
    });

    const std::string PATH_2 = "/test/2";
    const std::string MESSAGE_2 = "Another one...";

    s.set_endpoint(PATH_2, [&MESSAGE_2] (serv::Server* srv, serv::Context* ctx) {
        ctx->send_message(MESSAGE_2);
    });

    client.try_connect();
    client.handshake_init();
    client.handshake_final();

    using namespace serv::proto;
    Header header;
    header.set_type(Header_Type::Header_Type_TYPE_REQUEST);
    header.set_size(0);
    header.set_path(PATH_1);

    client.try_send(header.SerializeAsString());
    BOOST_ASSERT( client.try_recv() == MESSAGE_1 );

    header.set_path(PATH_2);

    client.try_send(header.SerializeAsString());
    BOOST_ASSERT( client.try_recv() == MESSAGE_2 );
}

BOOST_FIXTURE_TEST_CASE( server_basic_multiple_connection_test, ServerFixture ) {
    const std::string PATH = "/test";

    s.set_endpoint(PATH, [] (serv::Server* srv, serv::Context* ctx) {
        std::string data = "1";
        ctx->send_message(data);
    });

    constexpr int NCLIENTS = 100;
    std::vector<test::Client> clients(NCLIENTS);

    for (int i = 0; i < NCLIENTS; ++i) {
        clients[i] = test::Client("8000");
        clients[i].try_connect();
        clients[i].handshake_init();
        clients[i].handshake_final();

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ns);
    }

    for (int i = 0; i < NCLIENTS; ++i) {
        using namespace serv::proto;
        Header header;
        header.set_type(Header_Type::Header_Type_TYPE_REQUEST);
        header.set_size(0);
        header.set_path(PATH);

        clients[i].try_send(header.SerializeAsString());
    }

    for (int i = 0; i < NCLIENTS; ++i) {
        BOOST_ASSERT( clients[i].try_recv() == "1" );
    }
}