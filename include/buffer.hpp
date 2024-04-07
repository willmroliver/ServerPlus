#ifndef INCLUDE_BUFFER_H
#define INCLUDE_BUFFER_H

#include <string>
#include <utility>
#include <iostream>

namespace serv {

/**
 * @brief A circular buffer providing access to an underlying char array via a write function accepting a callback (allows zero-copy writes).
 * 
 * @tparam T The size of the underlying buffer
 */
template <unsigned T>
class Buffer {
    private:
        char buff[T];
        unsigned begin;
        unsigned end;
        bool full;

    public:
        Buffer():
            begin { 0 },
            end { 0 },
            full { false }
        {
            std::memset(buff, 0, T);
        };

        Buffer(std::vector<char>& data):
            begin { 0 },
            end { 0 },
            full { false }
        {   
            std::memset(buff, 0, T);

            for (; begin < data.size() && begin < T; ++begin) {
                buff[begin] = data[begin];
            }

            if (data.size() == T) {
                full = true;
                begin = 0;
            }
        }

        Buffer(std::string& data):
            begin { 0 },
            end { 0 },
            full { false }
        {   
            std::memset(buff, 0, T);

            for (; begin < data.size() && begin < T; ++begin) {
                buff[begin] = data[begin];
            }

            if (data.size() == T) {
                full = true;
                begin = 0;
            }
        }

        /**
         * @brief Reads from the underlying char array into a string and shifts the access pointers accordingly.
         * 
         * @param limit The max number of bytes to read. If negative, reads all available data (default behaviour).
         * @return The data read.
         */
        std::vector<char> read(int limit=-1) {
            std::vector<char> result;

            if (limit == 0 || is_empty()) {
                return result;
            }

            if (limit < 0) {
                limit = T + 1;
            }

            unsigned num_read = 0;

            do {
                result.push_back(buff[end]);
                buff[end] = 0;

                end = (end + 1) % T;
                ++num_read;
            } 
            while (begin != end && num_read < limit);

            full = false;
            
            return result;
        }

        /**
         * @brief Reads from the underlying char array, starting from the specified offset.
         * 
         * @param offset The number of bytes to pass over before reading from the buffer. Negative values are interpreted as offset from the end of the buffer.
         * @return The data read.
         */
        std::vector<char> read_from(int offset) {
            std::vector<char> result;

            if (is_empty()) {
                return result;
            }

            unsigned num_read = 0, start;

            if (offset >= 0) {
                start = end;

                for (int i = 0; i < offset; ++i) {
                    start = (start + 1) % T;
                    if (start == begin) {
                        return result;
                    }
                }
            } else {
                start = begin;

                for (int i = 0; i < -offset; ++i) {
                    start = (start - 1) % T;
                    if (start == begin) {
                        return result;
                    }
                }
            }

            do {
                result.push_back(buff[start]);
                buff[start] = 0;

                start = (start + 1) % T;
                ++num_read;
            } 
            while (begin != start);

            begin = (begin - num_read) % T;
            full = false;
            
            return result;
        }

        /**
         * @brief Reads from the underlying char array into a string and shifts the access pointers accordingly.
         * 
         * @param delim A delimiter character to stop reading at if encountered. If it is not found, the entire buffer content is read.
         * @return The resulting string & a boolean indicating whether the delimiter was found.
         */
        std::pair<std::vector<char>, bool> read_to(char delim) {
            std::vector<char> result;
            auto found = false;

            if (is_empty()) return { result, found };
            char c;

            do {
                c = buff[end];
                buff[end] = 0;

                if (c != delim) result.push_back(c);
                else found = true;
                
                end = (end + 1) % T;
            }
            while (begin != end && c != delim);

            full = false;

            return { result, found };
        }

        /**
         * @brief Copies the data passed into the buffer. If data size exceeds buffer space, no data is copied.
         * 
         * @param data The data to write copy into the buffer.
         * @return Success or failure
         */
        bool write(std::vector<char>& data) {
            if (data.size() == 0) {
                return true;
            }

            if (bytes_free() < data.size()) {
                return false;
            }

            for (auto i = 0; i < data.size(); ++i) {
                buff[begin] = data[i];
                begin = (begin + 1) % T;
            }

            full = begin == end;

            return true;
        }

        /**
         * @brief Calls the callback on the buffer. Intended for zero-copy buffer writes. 
         * Manages looping the circular buffer by applying the cb twice if the end is reached and there is space at the beginning.
         * 
         * @param cb A callback taking a char pointer and the number of bytes to write.
         * The callback should return 0 on failure, or the number of bytes written.
         * 
         * @param n The second argument of the callback.
         * 
         * @param data Additional data can be passed to the callback via this argument.
         * 
         * @return The number of bytes written and whether the buffer can still be written to.
         */
        std::pair<int, bool> write(int cb(char* dest, unsigned n, void* data), unsigned n, void* data = nullptr) {
            if (!can_write()) return { 0, false };
            if (!n) return { 0, true };

            if (begin < end || begin + n <= T) {
                auto bytes_read = cb(&buff[begin], n, data);
                begin = (begin + bytes_read) % T;
                full = begin == end;

                return { bytes_read, can_write() };
            } else {
                auto bytes1 = cb(&buff[begin], T - begin, data);
                if (bytes1 == 0) return { 0, true };

                begin = (begin + bytes1) % T;

                // We can't loop round as the buffer is full
                if (begin == end) {
                    full = true;
                    return { bytes1, false };
                }
                // We didn't read enough bytes to loop round
                else if (begin != 0) {
                    return { bytes1, true };
                }

                // Else we loop back to the start of the buffer and read again
                auto remaining = std::min(n - bytes1, end);

                auto bytes2 = cb(&buff[begin], remaining, data);
                begin = (begin + bytes2) % T;

                full = begin == end;

                return { bytes2 == 0 ? 0 : bytes1 + bytes2, can_write() };
            }
        }

        /**
         * @brief Returns the index from which the first instance of match begins in the underlying char array.
         * 
         * If none is found, returns string::npos
         * 
         * @param match 
         * @return The starting index of the discovered match, or string::npos
         */
        size_t find(std::string& match) const {
            auto n = match.length();

            if (
                is_empty()
                || (begin < end && end - begin < n) 
                || (begin > end && (T - begin + end) < n)
                || n == 0
            ) {
                return std::string::npos;
            }

            for (auto i = end; (i + n) % T != (begin + 1) % T; i = (i + 1) % T) {
                auto checking = i;
                
                for (auto j = 0; j < n; ++j) {
                    if (match[j] != *(buff + checking)) break;
                    if (j == n - 1) return i;

                    checking = (checking + 1) % T;
                }
            }

            return std::string::npos;
        }

        /**
         * @brief Tests whether the char exists in the written portion of the buffer.
         * 
         * @param match 
         * @return bool
         */
        bool contains(char match) const {            
            if (is_empty()) {
                return false;
            }

            auto i = end;

            do {
                if (buff[i] == match) {
                    return true;
                }
                i = (i + 1) % T;
            } 
            while (i != end);

            return false;
        }

        bool is_empty() const {
            return begin == end && !full;
        }

        bool can_write() const {
            return begin != end || !full;
        }

        unsigned size() const {
            if (full) return T;

            return begin < end 
            ? T - end + begin
            : begin - end;
        }

        unsigned bytes_free() const {
            if (full) return 0;

            return begin < end 
            ? end - begin
            : T - begin + end;
        }

        void clear() {
            std::memset(buff, 0, T + 1);
            begin = 0;
            end = 0;
            full = false;
        }
};

extern template class Buffer<16>;
using Buffer16 = Buffer<16>;

extern template class Buffer<1024>;
using Buffer1024 = Buffer<1024>;

extern template class Buffer<8192>;
using Buffer8192 = Buffer<8192>;

}

#endif