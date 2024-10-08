#ifndef INCLUDE_CIRCULAR_BUFFER_H
#define INCLUDE_CIRCULAR_BUFFER_H

#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include "logger.hpp"

namespace serv {

/**
 * @brief Circular Buffer implementation with support for delimited reads and zero-copy writes.
 * 
 */
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
        /**
         * @brief Create a new circular buffer of size `capacity`; must be a power of 2:
         * if an invalid capacity is passed, defaults to 1024.
         * 
         * @param capacity Must be a power of 2
         */
        CircularBuf(uint32_t capacity=1024);

        /**
         * @brief Create a new circular buffer and initialize with data.
         * 
         * @param data 
         * @param capacity Must be a power of 2
         */
        CircularBuf(std::vector<char>& data, uint32_t capacity=1024);

        /**
         * @brief Create a new circular buffer and initialize with data.
         * 
         * @param data 
         * @param capacity Must be a power of 2
         */
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

        /**
         * @brief Read the content of the buffer, optionally specifying a maximum number of bytes.
         * 
         * @param lim 
         * @return std::vector<char> 
         */
        std::vector<char> read(uint32_t lim=-1) noexcept;

        /**
         * @brief Read up to the first instance of the single-byte delimiter.
         * 
         * @param delim 
         * @return std::vector<char> 
         */
        std::vector<char> read_to(char delim) noexcept;

        /**
         * @brief Read up to the first instance of the multi-byte delimiter.
         * 
         * @param delim 
         * @return std::vector<char> 
         */
        std::vector<char> read_to(const std::string& delim) noexcept;

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

        /**
         * @brief Write data to the buffer.
         * 
         * @param data 
         * @return uint32_t 
         */
        uint32_t write(std::vector<char>& data);

        /**
         * @brief Write data to the buffer.
         * 
         * @param data 
         * @return uint32_t 
         */
        uint32_t write(std::vector<char>&& data);

        /**
         * @brief Write data to the buffer.
         * 
         * @param data 
         * @return uint32_t 
         */
        uint32_t write(std::string& data);

        /**
         * @brief Write data to the buffer.
         * 
         * @param data 
         * @return uint32_t 
         */
        uint32_t write(std::string&& data);        

        /**
         * @brief Provides direct access to the underlying byte-array via a callback function which
         * ought to define a write-procedure and should return the number of bytes written.
         * 
         * Minimizes unnecessary copying.
         * 
         * @param cb (char* dest, uint32_t n, void* data) The write-procedure. 
         * 
         * ---
         * 
         *  - `*dest` is a pointer to the first available byte in the buffer.
         * 
         *  - `n` is the number of bytes to write.
         * 
         *  - `data` allows us to pass through dependencies.
         * 
         * ---
         * @param n The number of bytes to write.
         * @param data Optional dependency injection passed through to the callback.
         * @return uint32_t 
         */
        uint32_t write(uint32_t cb(char* dest, uint32_t n, void* data) noexcept, uint32_t n, void* data=nullptr);

        void clear();
};

}

#endif