#include <cstring>
#include "circular-buffer.hpp"

namespace serv {

uint32_t CircularBuf::get_capacity(uint32_t c) const noexcept {
    if (c & (c - 1)) {
        Logger::get().error("buffer: constructor: capacity not power of 2, defaulting to 1024");
        return 1024;
    }

    return c;
}

uint32_t CircularBuf::mask(uint64_t i) const noexcept {
    return static_cast<uint32_t>(i & (capacity - 1));
}

bool CircularBuf::push(char b) noexcept {
    if (full()) {
        return false;
    }

    buf[mask(w++)] = b;
    return true;
}

bool CircularBuf::shift(char& b) noexcept {
    if (empty()) {
        return false;
    }

    b = buf[mask(r++)];
    return true;
}

CircularBuf::CircularBuf(uint32_t capacity): 
    r { 0 },
    w { 0 },
    capacity { get_capacity(capacity) }
{   
    buf = new char[capacity];
}

CircularBuf::CircularBuf(std::vector<char>& data, uint32_t capacity):
    CircularBuf(capacity)
{
    std::memcpy(buf, data.data(), static_cast<uint32_t>(data.size()));
}

CircularBuf::CircularBuf(std::string& data, uint32_t capacity):
    CircularBuf(capacity)
{
    std::memcpy(buf, data.c_str(), static_cast<uint32_t>(data.size()));
}

CircularBuf::CircularBuf(const CircularBuf& c):
    r { c.r },
    w { c.w },
    capacity { c.capacity },
    buf { new char[c.capacity] }
{
    std::memcpy(buf, c.buf, c.capacity);
}

CircularBuf::CircularBuf(CircularBuf&& c):
    r { c.r },
    w { c.w },
    capacity { c.capacity },
    buf { c.buf }
{
    c.r = 0;
    c.w = 0;
    c.capacity = 0;
    c.buf = nullptr;
}

CircularBuf& CircularBuf::operator=(const CircularBuf& c) {
    r = c.r;
    w = c.w;
    capacity = c.capacity;
    buf = new char[c.capacity];
    std::memcpy(buf, c.buf, c.capacity);

    return *this;
}

CircularBuf& CircularBuf::operator=(CircularBuf&& c) {
    r = c.r;
    w = c.w;
    capacity = c.capacity;
    buf = c.buf;

    c.r = 0;
    c.w = 0;
    c.capacity = 0;
    c.buf = nullptr;

    return *this;
}

CircularBuf::~CircularBuf() {
    delete[] buf;
}

uint32_t CircularBuf::size() const noexcept {
    return static_cast<uint32_t>(w - r);
}

uint32_t CircularBuf::space() const noexcept {
    return capacity - size();
}

bool CircularBuf::full() const noexcept {
    return capacity == size();
}

bool CircularBuf::empty() const noexcept {
    return r == w;
}

std::vector<char> CircularBuf::read(uint32_t lim) {
    if (lim < 0 || lim > size()) {
        lim = size();
    }

    std::vector<char> data(lim);
    uint32_t i = 0;

    while (i < lim && shift(data[i++]));

    return std::vector<char>(data.begin(), data.begin() + i);
}

std::vector<char> CircularBuf::read_to(char delim) {
    std::vector<char> data(size());
    uint32_t i = 0;
    bool success;

    while ((success = shift(data[i++])) && data[i-1] != delim);

    data.resize(i - !success);
    return data;
}

std::vector<char> CircularBuf::read_to(std::string delim) {
    std::vector<char> data(size());
    uint32_t nbytes = delim.size(), lim = size(), i = 0;
    bool success;

    auto test = [&] (std::vector<char> v, uint32_t from) {
        if (from + nbytes > lim) {
            return false;
        }

        return std::memcmp(delim.c_str(), v.data() + from, nbytes) == 0;
    };

    while ((success = shift(data[i++])) && !test(data, i-1));

    data.resize(i - !success);
    return data;
}

std::vector<char> CircularBuf::read_from(uint32_t offset) {
    if (offset >= size()) {
        return {};
    }

    uint32_t hold = r;
    r += offset;

    std::vector<char> data = read();

    r = hold;
    w = r + offset;

    return data;
}

uint32_t CircularBuf::write(std::vector<char>& data) {
    int n = 0;

    while (n < data.size() && push(data[n])) {
        ++n;
    };

    return n;
}

uint32_t CircularBuf::write(std::vector<char>&& data) {
    return write(data);
}

uint32_t CircularBuf::write(std::string& data) {
    return write(std::vector<char> { data.begin(), data.end() });
}

uint32_t CircularBuf::write(std::string&& data) {
    return write(std::vector<char> { data.begin(), data.end() });
}

uint32_t CircularBuf::write(uint32_t cb(char* dest, uint32_t n, void* data) noexcept, uint32_t n, void* data) {
    if (full()) {
        return 0;
    }

    uint32_t _r = mask(r), _w = mask(w);
    n = std::min(n, space());

    uint32_t shift;

    if (_w < _r || !_r) {
        shift = cb(buf + _w, n, data);
        w += shift;
        return shift;
    }

    shift = cb(buf + _w, std::min(n, capacity - _w), data);
    if (shift < n) {
        shift += cb(buf, std::min(n - shift, _r), data);
    }

    w += shift;
    return shift;
}

void CircularBuf::clear() {
    std::memset(buf, 0, capacity);
    r = w = 0;
}

}