#include <boost/test/unit_test.hpp>
#include <crypt/exchange.hpp>

#include "util.hpp"
#include "client.hpp"
#include "server.hpp"
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"

#include <thread>
#include <iostream>

struct F {
    static test::Client<1024> client;
    static serv::Server s;
    static std::thread t;

    void setup() {
        auto run = [=] () {
            s.run();
        }; 
        
        t = std::thread(run);
        t.detach();
    }

    void teardown() {
        s.stop();
    }
};

test::Client<1024> F::client { "3993" };
serv::Server F::s { "3993" };
std::thread F::t;

BOOST_FIXTURE_TEST_CASE( handshake_test, F ) {
    BOOST_ASSERT(F::client.try_connect());

    serv::proto::HostHandshake host_hs;

    BOOST_ASSERT(host_hs.ParseFromString(F::client.try_recv()));
    auto host_pk_str = host_hs.public_key();

    crpt::Exchange dh { "ffdhe2048" };
    crpt::PublicKeyDER host_pk;

    host_pk.from_vector({ host_pk_str.begin(), host_pk_str.end() });
    serv::proto::PeerHandshake peer_hs;

    auto peer_pk = dh.get_public_key().to_vector();
    peer_hs.set_public_key({ peer_pk.begin(), peer_pk.end() });

    BOOST_ASSERT(F::client.try_send(peer_hs.SerializeAsString()));

    auto res = F::client.try_recv();
    BOOST_ASSERT(res.size() == 1 && res[0] == 1);

    BOOST_ASSERT(F::client.try_close());
}