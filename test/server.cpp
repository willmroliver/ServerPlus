#include <boost/test/unit_test.hpp>
#include <event2/event.h>

#include "util.hpp"
#include "client.hpp"
#include "server.hpp"
#include "header.pb.h"

#include <thread>
#include <iostream>

struct GlobalFixture {
    static test::Client<1024> client;
    static serv::Server s;
    static std::thread t;

    void setup() {
        BOOST_TEST_MESSAGE("initializing server");

        auto run = [=] () {
            s.run();
        }; 
        
        t = std::thread(run);
        t.detach();
    }

    void teardown() {
        s.stop();

        BOOST_ASSERT(s.get_status() != -1);
        BOOST_TEST_MESSAGE("client, server tests complete");
    }
};

test::Client<1024> GlobalFixture::client { "3993" };
serv::Server GlobalFixture::s { "3993" };
std::thread GlobalFixture::t;

BOOST_TEST_GLOBAL_FIXTURE( GlobalFixture );

BOOST_AUTO_TEST_CASE( ping_test, * boost::unit_test::description("test server responses to correct & malformed headers") ) {
    BOOST_ASSERT(GlobalFixture::client.try_connect());

    serv::proto::Header header;
    header.set_type("ping");
    header.set_size(0);

    auto time_start = test::Util::current_timestamp<std::chrono::milliseconds>();
    header.set_timestamp(time_start);

    BOOST_ASSERT(GlobalFixture::client.try_send(header.SerializeAsString()));
    BOOST_ASSERT(header.ParseFromString(GlobalFixture::client.try_recv()));
    BOOST_ASSERT(time_start == header.timestamp());

    BOOST_ASSERT(GlobalFixture::client.try_close());
}