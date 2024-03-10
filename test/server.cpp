#include <boost/test/unit_test.hpp>
#include <crypt/exchange.hpp>

#include "utility/time.hpp"
#include "client.hpp"
#include "server.hpp"
#include "logger.hpp"
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"
#include "header.pb.h"

#include <thread>
#include <iostream>

struct ServerFixture {
    test::Client<1024> client;
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

    auto ping_ts = serv::util::sys_timestamp<std::chrono::nanoseconds>;

    Header ping_header;
    ping_header.set_type(Header_Type::Header_Type_TYPE_PING);
    ping_header.set_size(0);
    ping_header.set_timestamp(ping_ts());

    BOOST_ASSERT(client.try_send(ping_header.SerializeAsString()));

    auto res = client.try_recv();
    BOOST_ASSERT(ping_header.ParseFromString(res));

    serv::Logger::get().log("PING: " + std::to_string(ping_ts() - ping_header.timestamp()));
}