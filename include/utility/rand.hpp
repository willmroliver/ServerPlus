#ifndef INCLUDE_UTILITY_RAND_H
#define INCLUDE_UTILITY_RAND_H

#include <string>
#include <random>

namespace serv {
namespace util {

/**
 * @brief Pseudorandom alpha-numeric string generation
 * 
 * @param n Result length
 * @return std::string 
 */
std::string rand_alphanumeric(int n) {
    static const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int chars_len = chars.size();

    std::random_device rd;
    std::mt19937 mer(rd());
    std::uniform_int_distribution dist(0, chars_len - 1);

    std::string data(n);

    for (auto i = 0; i < n; ++i) {
        data[i] = chars[dist(mer)];
    }

    return data;
}

void rand_alphanumeric_inplace(std::string& data) {
    static const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int chars_len = chars.size();

    int n = data.size();

    std::random_device rd;
    std::mt19937 mer;
    std::uniform_int_distribution dist(0, chars_len - 1);

    for (auto i = 0; i < n; ++i) {
        data[i] = chars[dist(mer)];
    }
}


/**
 * @brief Pseudorandom string generation
 * 
 * @param n Result length
 * @param null_byte If true, permits null bytes in the result
 * @return std::string 
 */
std::string rand_utf8(int n, bool null_byte=false) {
    std::random_device rd;
    std::mt19937 mer(rd());
    std::uniform_int_distribution dist(null_byte ? 0 : 1, 255);

    std::string data(n);

    for (auto i = 0; i < n; ++i) {
        data[i] = dist(mer);
    }

    return data;
}

void rand_utf8_inplace(std::string& data, bool null_byte=false) {
    int n = data.size();

    std::random_device rd;
    std::mt19937 mer;
    std::uniform_int_distribution dist(null_byte ? 0 : 1, 255);

    for (auto i = 0; i < n; ++i) {
        data[i] = dist(mer);
    }
}

}

}

#endif