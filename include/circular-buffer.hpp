#ifndef INCLUDE_CIRCULAR_BUFFER_H
#define INCLUDE_CIRCULAR_BUFFER_H

#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include "logger.hpp"

namespace serv {

class CircularBuf {
    private:
        char* buf;
        uint64_t r;
        uint64_t w;
        uint32_t capacity;

        uint32_t get_capacity(uint32_t c) const noexcept;

        uint32_t mask(uint64_t i) const noexcept;

        bool push(char b) noexcept;

        bool shift(char& b) noexcept;
    public:
        CircularBuf(uint32_t capacity=1024);

        CircularBuf(std::vector<char>& data, uint32_t capacity=1024);

        CircularBuf(std::string& data, uint32_t capacity=1024);

        CircularBuf(const CircularBuf& c);

        CircularBuf(CircularBuf&& c);

        ~CircularBuf();

        CircularBuf& operator=(const CircularBuf& c);

        CircularBuf& operator=(CircularBuf&& c);

        uint32_t size() const noexcept;

        uint32_t space() const noexcept;

        bool full() const noexcept;

        bool empty() const noexcept;

        std::vector<char> read(uint32_t lim=-1);

        std::vector<char> read_to(char delim);

        /**
         * @brief Read all bytes in the underlying buffer from the offset, and move the write pointer to 
         * the offset start.
         * 
         * All data from the offset onwards is cleared to avoid discontinuous data / overwrites. Allows
         * us to pop and replace incoming cipher text with plain text in the same buffer.
         * 
         * @param offset 
         * @return std::vector<char> 
         */
        std::vector<char> read_from(uint32_t offset);

        uint32_t write(std::vector<char>& data);

        uint32_t write(std::vector<char>&& data);

        uint32_t write(std::string& data);

        uint32_t write(std::string&& data);        

        uint32_t write(uint32_t cb(char* dest, uint32_t n, void* data) noexcept, uint32_t n, void* data=nullptr);

        void clear();
};

}

#endif