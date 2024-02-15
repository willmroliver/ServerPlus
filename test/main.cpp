#define BOOST_TEST_MODULE server_test_suite

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <crypt/error.hpp>
#include "logger.hpp"

struct GlobalFixture {
    
    std::ofstream tout { "test/zout.txt", std::ios::out | std::ios::trunc };
    std::ofstream terr { "test/zerr.txt", std::ios::out | std::ios::trunc };

    void setup() {
        serv::Logger::get();
        serv::Logger::set(&tout, &terr);
        crpt::Error::set_err_ostream(serv::Logger::get().get_err_stream());
    }

    void teardown() {
        tout.close();
        terr.close();
    }
};

BOOST_TEST_GLOBAL_FIXTURE( GlobalFixture );