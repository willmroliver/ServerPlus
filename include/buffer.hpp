#ifndef INCLUDE_BUFFER_H
#define INCLUDE_BUFFER_H

#include <string>
#include <utility>

namespace serv {

/**
 * @brief A circular buffer providing access to an underlying char array via a write function accepting a callback (allows zero-copy writes).
 * 
 * @tparam T The size of the underlying buffer
 */
template <unsigned T>
class Buffer {
    private:
        char buff[T + 1];
        unsigned begin;
        unsigned end;
        bool full;

    public:
        Buffer():
            begin { 0 },
            end { 0 },
            full { false }
        {
            std::memset(buff, 0, T + 1);
        };

        Buffer(std::string data):
            begin { 0 },
            end { 0 },
            full { false }
        {   
            std::memset(buff, 0, T + 1);

            for (; begin < data.size() && begin < T; ++begin) {
                buff[begin] = data[begin];
            }

            if (data.size() == T) {
                full = true;
                begin = 0;
            }
        }

        Buffer(Buffer& b):
            begin { b.begin },
            end { b.end },
            full { b.full }
        {
            std::memcpy(buff, b.buff, sizeof(buff));
        }

        Buffer(Buffer&& b):
            begin { b.begin },
            end { b.end },
            full { b.full }
        {
            buff = b.buff;
            b.buff = nullptr;

            b.begin = 0;
            b.end = 0;
            b.full = false;
        }

        ~Buffer() = default;

        /**
         * @brief Reads all available data from the underlying char array into a string and shifts the access pointers accordingly.
         * 
         * @return std::string The resulting string.
         */
        std::string read() {
            if (empty()) return "";

            std::string result;
            
            do {
                result.push_back(buff[end]);
                buff[end] = 0;
                end = (end + 1) % T;
            } 
            while (begin != end);

            full = false;

            return result;
        }

        /**
         * @brief Reads from the underlying char array into a string and shifts the access pointers accordingly.
         * 
         * @param len The max number of bytes to read. Will ignore any requested bytes greater than present in the array.
         * @return std::string The resulting string.
         */
        std::string read(unsigned len) {
            std::string result;

            if (len == 0 || empty()) return result;

            auto num_read = 0;

            do {
                result.push_back(buff[end]);
                buff[end] = 0;

                end = (end + 1) % T;
                ++num_read;
            } 
            while (begin != end && num_read < len);

            full = false;
            
            return result;
        }

        /**
         * @brief Reads from the underlying char array into a string and shifts the access pointers accordingly.
         * 
         * @param delim A delimiter character to stop reading at if encountered. If it is not found, the entire buffer content is read.
         * @return std::pair<std::string, bool> The resulting string & a boolean indicating whether the delimiter was found.
         */
        std::pair<std::string, bool> read_to(char delim) {
            std::string result;
            auto found = false;

            if (empty()) return { result, found };
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
         * @brief Returns the index from which the first instance of match begins in the underlying char array.
         * 
         * If none is found, returns string::npos
         * 
         * @param match 
         * @return size_t The starting index of the discovered match, or string::npos
         */
        size_t find(std::string match) const {
            auto n = match.length();
            if (empty()
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
         * @return std::pair<int, bool> The number of bytes written and whether the buffer can still be written to.
         */
        std::pair<int, bool> write(int cb(char* dest, unsigned n, void* data), unsigned n, void* data = nullptr) {
            if (!can_write()) return { 0, false };
            if (!n) return { 0, true };

            if (begin < end || begin + n < T) {
                auto bytes_read = cb(&buff[begin], n, data);
                begin = begin + bytes_read;
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
                begin += bytes2;

                full = begin == end;

                return { bytes2 == 0 ? 0 : bytes1 + bytes2, can_write() };
            }
        }

        bool empty() const {
            return begin == end && !full;
        }

        bool can_write() const {
            return begin != end || !full;
        }

        unsigned bytes_free() const {
            if (full) return 0;

            return begin < end 
            ? end - begin
            : T - begin + end;
        }
};

}

#endif