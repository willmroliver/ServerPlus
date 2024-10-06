#include <boost/test/unit_test.hpp>
#include <string>
#include "circular-buffer.hpp"

namespace {

/**
 * @brief Mocks a call to recvfrom()
 * 
 * @param socket 
 * @param buffer 
 * @param length 
 * @return int 
 */
int mock_recvfrom(int socket, void* buffer, size_t length) {
    static char data[9] = "12345678";

    for (auto i = 0; i < length; ++i) {
        ((char*)buffer)[i] = data[((i % 8) + 8) % 8];
    }

    return length;
}

/**
 * @brief The callback to pass to buffer's write() when reading from the above mock_recvfrom().
 * 
 * @param buff 
 * @param n 
 * @param data 
 * @return uint32_t 
 */
uint32_t write_cb(char* buff, uint32_t n, void* data) noexcept {
    return mock_recvfrom(4, buff, n);
}

/**
 * @brief Artificially adds an offset to the buffer's internal pointers.
 * Useful to test function behaviour & conditions over the looping bound of the internal array.
 * 
 * @tparam T The buffer size
 * @param b The buffer
 * @param offset The offset size
 */
void offset_buffer(serv::CircularBuf& b, unsigned offset) {
    b.write([] (char* dest, uint32_t n, void* data) noexcept -> uint32_t {
        for (auto i = 0; i < n; ++i) {
            *(dest + i) = '0' + i % 10;
        }

        return n;
    }, offset, nullptr);

    b.read(offset);
}

struct WriteTestCase {
    std::string initial;
    uint32_t write_size;
    uint32_t expecting_size;
    bool expecting_full;
};

WriteTestCase write_tests[] = {
    {"", 0, 0, false},
    {"", 8, 8, false},
    {"", 17, 16, true},
    {"12345678", 8, 16, true},
    {"1234567890123456", 8, 16, true},
};

void do_write_test(WriteTestCase& test) {
    serv::CircularBuf buffer(16);
    buffer.write(test.initial);

    auto space = buffer.space();
    BOOST_ASSERT( space == 16 - test.initial.size() );

    auto n = buffer.write(write_cb, test.write_size);

    BOOST_ASSERT( n == std::min(test.write_size, space));
    BOOST_ASSERT( buffer.size() == test.expecting_size );
    BOOST_ASSERT( buffer.full() == test.expecting_full );
}

BOOST_AUTO_TEST_CASE( circ_buf_write_table_test ) {
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
    serv::CircularBuf buffer(16);
    buffer.write(test.initial);

    auto bytes = buffer.read(test.read_size);
    std::string res { bytes.begin(), bytes.end() };

    BOOST_ASSERT( res == test.expecting );
    BOOST_ASSERT( buffer.empty() == test.expecting_empty );
}

BOOST_AUTO_TEST_CASE ( circ_buf_read_table_test ) {
    for (auto &test : read_tests ) do_read_test(test);
}

/**
 * @brief Test read & write calls in sequence.
 * 
 * Expected buffer state is commented. 
 * 
 * '>' indicates the circular buffer 'begin' pointer.
 * '|' indicates the end.
 * 
 */
BOOST_AUTO_TEST_CASE ( circ_buf_read_write_test ) {
    serv::CircularBuf buffer(16);

    // in buffer: |1234 5678 1234 5678>
    BOOST_ASSERT( buffer.write(write_cb, 19) == 16 );
    BOOST_ASSERT( buffer.full() && !buffer.empty() );

    // in buffer: |>---- ---- ---- ----
    auto bytes = buffer.read(100);
    std::string res { bytes.begin(), bytes.end() };

    BOOST_ASSERT( res.size() == 16 );
    BOOST_ASSERT( res == "1234567812345678\0" );
    BOOST_ASSERT( buffer.space() == 16 );

    // in buffer: |1234 5678 1234> ----
    buffer.write(write_cb, 12);
    
    // in buffer: ---- ---- |1234> ----
    bytes = buffer.read(8);
    res = { bytes.begin(), bytes.end() };

    BOOST_ASSERT( res == "12345678\0" );
    BOOST_ASSERT( buffer.space() == 12 );

    // in buffer: 1234 5678> |1234 1234
    buffer.write(write_cb, 12);
    /**
     * due to nature of mock_recvfrom,
     * when buffer loops round the data input sequence starts again, 
     * hence the repeated reading of 1234 above.
    */

    BOOST_ASSERT( buffer.full() && !buffer.empty() );

    // in buffer: 1234 5678> ---- --|34
    bytes = buffer.read(6);
    res = { bytes.begin(), bytes.end() };

    BOOST_ASSERT( res == "123412\0" );
    BOOST_ASSERT( buffer.space() == 6 );

    // in buffer: |>---- ---- ---- ----
    bytes = buffer.read(10);
    res = { bytes.begin(), bytes.end() };

    BOOST_ASSERT( res == "3412345678\0" );
    BOOST_ASSERT( !buffer.full() && buffer.empty() );
    BOOST_ASSERT( buffer.space() == 16 );
}

struct ReadToTestCase {
    unsigned offset;
    std::string initial;
    char delim;
    std::string expecting;
};

ReadToTestCase read_to_tests[] = {
    {0, "12345678", '1', "1"},
    {0, "12345678", '2', "12"},
    {0, "12345678", '9', "12345678"},
    {0, "12345678", 0, "12345678"},
    {12, "12345678", '1', "1"},
    {12, "12345678", '2', "12"},
    {12, "12345678", '9', "12345678"},
    {12, "12345678", 0, "12345678"},
    {16, "12345678", '1', "1"},
    {16, "12345678", '2', "12"},
    {16, "12345678", '9', "12345678"},
    {16, "12345678", 0, "12345678"},
};

void do_read_to_test(ReadToTestCase& test) {
    serv::CircularBuf buffer(16);
    offset_buffer(buffer, test.offset);

    // Let's test out another callback write operation while we're here
    buffer.write([] (char* dest, uint32_t n, void* data) noexcept -> uint32_t {
        auto test = ((ReadToTestCase*) data);

        for (auto i = 0; i < n; ++i) {
            // Mimic some kind of source-cannibalisation
            *(dest + i) = test->initial[0];
            test->initial.erase(0, 1);
        }

        return n;
    },  test.initial.size(), &test);

    auto bytes = buffer.read_to(test.delim);
    std::string result { bytes.begin(), bytes.end() };

    if (result != test.expecting) {
        std::cout 
        << "circ_buf_read_to_table_test failed: reading until '" << test.delim 
        << "', expecting "<< test.expecting 
        << ", got " << result 
        << std::endl;
    }

    BOOST_ASSERT( result == test.expecting );
}

BOOST_AUTO_TEST_CASE( circ_buf_read_to_table_test ) {
    for (auto &test : read_to_tests) do_read_to_test(test);
}

}