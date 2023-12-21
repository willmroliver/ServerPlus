#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/bind/bind.hpp>
#include <string>

#include "buffer.hpp"

int mock_recvfrom(int socket, void* buffer, size_t length) {
    static char data[9] = "12345678";

    for (auto i = 0; i < length; ++i) {
        ((char*)buffer)[i] = data[((i % 8) + 8) % 8];
    }

    return length;
}

int write_cb(char* buff, unsigned n) {
    return mock_recvfrom(4, buff, n);
}

struct WriteTestCase {
    std::string initial;
    size_t write_size;
    size_t expecting_size;
    bool expecting_full;
};

WriteTestCase write_tests[] = {
    {"", 0, 0, false},
    {"", 8, 8, false},
    {"", 17, 16, true},
    {"12345678", 8, 8, true},
    {"1234567890123456", 8, 0, true},
};

void do_write_test(WriteTestCase& test) {
    serv::Buffer<16> buffer (test.initial);

    auto [n, can_write] = buffer.write(write_cb, test.write_size);

    BOOST_ASSERT( n == test.expecting_size );
    BOOST_ASSERT( can_write != test.expecting_full );
    BOOST_ASSERT( buffer.can_write() != test.expecting_full );
}

BOOST_AUTO_TEST_CASE( buffer_write_table_test ) {
    for (auto &test : write_tests) do_write_test(test);
}

struct ReadTestCase {
    std::string initial;
    size_t read_size;
    std::string expecting;
    bool expecting_empty;
};

ReadTestCase read_tests[] = {
    {"", 0, "", true},
    {"", 4, "", true},
    {"1234", 4, "1234", true},
    {"12345678", 3, "123", false},
};

void do_read_test(ReadTestCase& test) {
    serv::Buffer<16> buffer (test.initial);

    std::string result = buffer.read(test.read_size);
    BOOST_ASSERT( result == test.expecting );
    BOOST_ASSERT( buffer.empty() == test.expecting_empty );
}

BOOST_AUTO_TEST_CASE ( buffer_read_table_test ) {
    for (auto &test : read_tests ) do_read_test(test);
}

/**
 * @brief Integration test of read & write calls in sequence.
 * 
 * Expected buffer state is commented. 
 * 
 * '|' indicates the circular buffer 'begin' pointer.
 * '>' indicates the end.
 * 
 */
BOOST_AUTO_TEST_CASE ( buffer_integration_test ) {
    serv::Buffer<16> buffer;

    // in buffer: |1234 5678 1234 5678>
    auto [n, can_write] = buffer.write(write_cb, 19);
    BOOST_ASSERT( n == 16 );
    BOOST_ASSERT( !can_write );

    // in buffer: |> ---- ---- ---- ----
    std::string result = buffer.read(100);
    BOOST_ASSERT( result.size() == 16 );
    BOOST_ASSERT( result == "1234567812345678\0" );

    // in buffer: |1234 5678 1234> ----
    buffer.write(write_cb, 12);
    
    // in buffer: ---- ---- |1234> ----
    result = buffer.read(8);
    BOOST_ASSERT( result == "12345678\0" );

    // in buffer: 1234 5678> |1234 1234
    buffer.write(write_cb, 12);
    /**
     * due to nature of mock_recvfrom,
     * when buffer loops round the data input sequence starts again, 
     * hence the repeated reading of 1234 above.
    */

    BOOST_ASSERT( !buffer.empty() );
    BOOST_ASSERT( !buffer.can_write() );

    // in buffer: 1234 5678> ---- --|34
    result = buffer.read(6);
    BOOST_ASSERT( result == "123412\0" );

    // in buffer: 
    result = buffer.read(10);
    BOOST_ASSERT( result == "3412345678\0" );
    BOOST_ASSERT( buffer.empty() );
    BOOST_ASSERT( buffer.can_write() );
}