#ifndef INCLUDE_FIXTURE_H
#define INCLUDE_FIXTURE_H

#include <boost/test/unit_test.hpp>
#include <thread>

#include "client.hpp"
#include "server.hpp"

struct GlobalFixture {
    static test::Client<1024> client;
    static db::Server s;
    static std::thread t;

    void setup() {
        std::cout << "setting up" << std::endl;
        BOOST_TEST_MESSAGE("initializing server");

        auto run = [=] () {
            s.run();
        }; 
        
        t = std::thread(run);
        t.detach();
    }

    void teardown() {
        s.stop();
        BOOST_ASSERT(s.get_status() == 1);
        BOOST_TEST_MESSAGE("client, server tests complete");
    }
};

test::Client<1024> GlobalFixture::client { "3993" };
db::Server GlobalFixture::s { "3993" };
std::thread GlobalFixture::t;

#endif