#include <boost/test/unit_test.hpp>
#include <event2/event.h>

#include "client.hpp"
#include "server.hpp"

#include <thread>

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

BOOST_AUTO_TEST_CASE( simple_handler_test, * boost::unit_test::description("test client connects to server") ) {
    BOOST_ASSERT(GlobalFixture::client.try_connect());
    BOOST_ASSERT(GlobalFixture::client.try_close());
}
