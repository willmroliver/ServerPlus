#ifndef INCLUDE_HELPERS_H
#define INCLUDE_HELPERS_H

#include <iostream>
#include <vector>
#include <thread>
#include <boost/test/unit_test.hpp>
#include "logger.hpp"

inline void clear_logger() {
    serv::Logger::get().clear_buf();
}

/**
 * @brief A little time buffer is often useful to ensure OS network stack has resolved in time for non-blocking sock operations to be called.
 */
inline void tiny_sleep() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ns);
}

inline void ASSERT_ERR_LOGGED(int err_code) {
    BOOST_ASSERT( serv::Logger::get().search_buf(err_code).size() );
}

inline void ASSERT_ERR_LOGGED(std::vector<int> err_codes) {
    for (const auto err_code : err_codes) {
        BOOST_ASSERT( serv::Logger::get().search_buf(err_code).size() );
    }
}

template <typename T>
inline void RUN_TEST_CASES(std::function<void(const T&)> cb, std::vector<T> tests) {
    for (const auto& test : tests) {
        cb(test);
    }
}

template <typename T, typename... Args>
inline void RUN_TEST_CASES(void cb(const T&, Args&...), std::vector<T> tests, Args&... args) {
    for (const auto& test : tests) {
        cb(test, args...);
    }
}

#endif