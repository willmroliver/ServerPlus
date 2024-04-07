#include <boost/test/unit_test.hpp>
#include <string>
#include "buffer.hpp"

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
 * @return int 
 */
int write_cb(char* buff, unsigned n, void* data) {
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
template <unsigned T>
void offset_buffer(serv::Buffer<T>& b, unsigned offset) {
    b.write([] (char* dest, unsigned n, void* data) -> int {
        for (auto i = 0; i < n; ++i) {
            *(dest + i) = '0' + i % 10;
        }

        return n;
    }, offset, nullptr);

    b.read(offset);
}

template void offset_buffer<16>(serv::Buffer16& b, unsigned offset);

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
    serv::Buffer16 buffer (test.initial);

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
    serv::Buffer16 buffer (test.initial);

    auto res_bytes = buffer.read(test.read_size);
    std::string result { res_bytes.begin(), res_bytes.end() };

    BOOST_ASSERT( result == test.expecting );
    BOOST_ASSERT( buffer.is_empty() == test.expecting_empty );
}

BOOST_AUTO_TEST_CASE ( buffer_read_table_test ) {
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
BOOST_AUTO_TEST_CASE ( buffer_read_write_test ) {
    serv::Buffer16 buffer;

    // in buffer: |1234 5678 1234 5678>
    auto [n, can_write] = buffer.write(write_cb, 19);
    BOOST_ASSERT( n == 16 );
    BOOST_ASSERT( !can_write );
    BOOST_ASSERT( !buffer.bytes_free() );

    // in buffer: |>---- ---- ---- ----
    auto res_bytes = buffer.read(100);
    std::string result = { res_bytes.begin(), res_bytes.end() };

    BOOST_ASSERT( result.size() == 16 );
    BOOST_ASSERT( result == "1234567812345678\0" );
    BOOST_ASSERT( buffer.bytes_free() == 16 );

    // in buffer: |1234 5678 1234> ----
    buffer.write(write_cb, 12);
    
    // in buffer: ---- ---- |1234> ----
    res_bytes = buffer.read(8);
    result = { res_bytes.begin(), res_bytes.end() };

    BOOST_ASSERT( result == "12345678\0" );
    BOOST_ASSERT( buffer.bytes_free() == 12 );

    // in buffer: 1234 5678> |1234 1234
    buffer.write(write_cb, 12);
    /**
     * due to nature of mock_recvfrom,
     * when buffer loops round the data input sequence starts again, 
     * hence the repeated reading of 1234 above.
    */

    BOOST_ASSERT( !buffer.is_empty() );
    BOOST_ASSERT( !buffer.can_write() );

    // in buffer: 1234 5678> ---- --|34
    res_bytes = buffer.read(6);
    result = { res_bytes.begin(), res_bytes.end() };

    BOOST_ASSERT( result == "123412\0" );
    BOOST_ASSERT( buffer.bytes_free() == 6 );

    // in buffer: 
    res_bytes = buffer.read(10);
    result = { res_bytes.begin(), res_bytes.end() };

    BOOST_ASSERT( result == "3412345678\0" );
    BOOST_ASSERT( buffer.is_empty() );
    BOOST_ASSERT( buffer.can_write() );
    BOOST_ASSERT( buffer.bytes_free() == 16 );
}

struct FindTestCase {
    unsigned offset;
    std::string initial;
    std::string search;
    size_t expecting;
};

FindTestCase find_tests[] = {
    {0, "0123456789", "A", std::string::npos},
    {0, "0123456789", "0", 0},
    {0, "0123456789", "9", 9},
    {0, "0123456789", "5", 5},
    {0, "0123456789", "012", 0},
    {0, "0123456789", "789", 7},
    {0, "0123456789", "345", 3},
    {10, "0123456789", "345", 13},
    {12, "01234567", "34", 15},
    {0, "", "", std::string::npos},
    {16, "", "34", std::string::npos},
};

void do_find_test(FindTestCase& test) {
    serv::Buffer16 buffer;

    if (test.offset) {
        offset_buffer<16>(buffer, test.offset);
    }

    buffer.write([] (char* dest, unsigned n, void* data) -> int {
        auto test = (FindTestCase*) data;

        for (auto i = 0; i < n; ++i) {
            *(dest + i) = test->initial[0];
            test->initial.erase(0, 1);
        }

        return n;
    },  test.initial.size(), &test);

    auto result = buffer.find(test.search);

    if (result != test.expecting) {
        std::cout 
        << "buffer_find_table_test failed: searching for '" 
        << test.search 
        << "', expecting "
        << test.expecting 
        << ", got " 
        << result 
        << std::endl;
    }

    BOOST_ASSERT( result == test.expecting );
}

BOOST_AUTO_TEST_CASE( buffer_find_table_test ) {
    for (auto &test : find_tests) do_find_test(test);
}

struct ReadToTestCase {
    unsigned offset;
    std::string initial;
    char delim;
    std::string expecting;
};

ReadToTestCase read_to_tests[] = {
    {0, "12345678", '1', ""},
    {0, "12345678", '2', "1"},
    {0, "12345678", '9', "12345678"},
    {0, "12345678", 0, "12345678"},
    {12, "12345678", '1', ""},
    {12, "12345678", '2', "1"},
    {12, "12345678", '9', "12345678"},
    {12, "12345678", 0, "12345678"},
    {16, "12345678", '1', ""},
    {16, "12345678", '2', "1"},
    {16, "12345678", '9', "12345678"},
    {16, "12345678", 0, "12345678"},
};

void do_read_to_test(ReadToTestCase& test) {
    serv::Buffer16 buffer;

    if (test.offset) {
        offset_buffer<16>(buffer, test.offset);
    }

    buffer.write([] (char* dest, unsigned n, void* data) -> int {
        auto test = (ReadToTestCase*) data;

        for (auto i = 0; i < n; ++i) {
            *(dest + i) = test->initial[0];
            test->initial.erase(0, 1);
        }

        return n;
    },  test.initial.size(), &test);

    auto [res_bytes, found] = buffer.read_to(test.delim);
    std::string result { res_bytes.begin(), res_bytes.end() };

    if (result != test.expecting) {
        std::cout 
        << "buffer_find_table_test failed: reading until '" << test.delim 
        << "', expecting "<< test.expecting 
        << ", got " << result 
        << std::endl;
    }

    BOOST_ASSERT( result == test.expecting );
}

BOOST_AUTO_TEST_CASE( buffer_read_to_table_test ) {
    for (auto &test : read_to_tests) do_read_to_test(test);
}

struct ReadFromTestCase {
    int arg;
    unsigned offset;
    std::string initial;
    std::string expecting;
};

ReadFromTestCase read_from_tests[] = {
    { 0, 0, "", "" },
    { 1, 0, "", "" },
    { 1, 0, "1", "" },
    { 4, 0, "1234", "" },
    { 4, 4, "12345678", "5678" },
    { 4, 12, "123456789999", "56789999" },
    { 4, 15, "123456789999", "56789999" },
    { 4, 16, "123456789999", "56789999" },
    { 4, 17, "123456789999", "56789999" },
    { 1, 4, "123456789999", "23456789999" },
    { 1, 12, "123456789999", "23456789999" },
    { 1, 15, "123456789999", "23456789999" },
    { 1, 16, "123456789999", "23456789999" },
    { 1, 17, "123456789999", "23456789999" },
    { 1, 15, "1", "" },
    { 1, 16, "1", "" },
    { 1, 17, "1", "" },
    { 0, 15, "1", "1" },
    { 0, 16, "1", "1" },
    { 0, 17, "1", "1" },
    { 1, 15, "12", "2" },
    { 1, 16, "12", "2" },
    { 1, 17, "12", "2" },
    { 0, 16, "12", "12" },
    { 0, 15, "12", "12" },
    { 0, 17, "12", "12" },
    { 4, 128 + 15, "123456789999", "56789999" },
    { 4, 128 + 16, "123456789999", "56789999" },
    { 4, 128 + 17, "123456789999", "56789999" },
    { 1, 128 + 4, "123456789999", "23456789999" },
    { 1, 128 + 12, "123456789999", "23456789999" },
    { 1, 128 + 15, "123456789999", "23456789999" },
    { 1, 128 + 16, "123456789999", "23456789999" },
    { 1, 128 + 17, "123456789999", "23456789999" },
    { 1, 128 + 16, "1", "" },
    { 1, 128 + 16, "12", "2" },
    { 1, 128 + 15, "1", "" },
    { -1, 128 + 4, "123456789999", "9" },
    { -2, 128 + 12, "123456789999", "99" },
    { -3, 128 + 15, "123456789999", "999" },
    { -4, 128 + 16, "123456789999", "9999" },
    { -12, 128 + 17, "123456789999", "123456789999" },
    { -1, 0, "", "" },
    { -1, 0, "1", "1" },
    { -4, 0, "1234", "1234" },
    { -4, 4, "12345678", "5678" },
    { -4, 12, "123456789999", "9999" },
};

void do_read_from_test(ReadFromTestCase& test) {
    serv::Buffer16 buffer;
    offset_buffer<16>(buffer, test.offset);

    std::vector<char> data(test.initial.begin(), test.initial.end());
    buffer.write(data);

    auto res_bytes = buffer.read_from(test.arg);
    std::string result { res_bytes.begin(), res_bytes.end() };
    
    if (result != test.expecting) {
        std::cout 
        << "buffer_find_table_test failed: reading from '" << test.arg
        << "', expecting '"<< test.expecting << "' len " << test.expecting.size()
        << ", got '" << result << "' len " << result.size()
        << std::endl;
    }

    BOOST_ASSERT( result == test.expecting );
}

BOOST_AUTO_TEST_CASE( buffer_read_from_table_test ) {
    for (auto &test : read_from_tests) do_read_from_test(test);
}
