#include <iostream>
#include <string>

#include "socket.hpp"
#include "logger.hpp"
#include "error-codes.hpp"

using namespace serv;

Socket::Socket(): 
    fd { 0 },
    listening { false }
{
    addr_len = sizeof addr;
    std::memset(&addr, 0, addr_len);
}

Socket::Socket(Socket& sock): 
    fd { sock.fd },
    listening { sock.listening },
    addr { sock.addr },
    addr_len { sock.addr_len },
    buf { sock.buf }
{}

Socket::Socket(Socket&& sock): 
    fd { sock.fd },
    listening { sock.listening },
    addr { sock.addr },
    addr_len { sock.addr_len },
    buf { sock.buf }
{
    sock.fd = 0;
    sock.listening = false;
    std::memset(&sock.addr, 0, sock.addr_len);
    sock.addr_len = 0;
    sock.buf.clear();
}

Socket& Socket::operator=(Socket& sock) {
    fd = sock.fd;
    listening = sock.listening;
    addr = sock.addr;
    addr_len = sock.addr_len;
    buf = sock.buf;
    return *this;
}

Socket& Socket::operator=(Socket&& sock) {
    fd = sock.fd;
    listening = sock.listening;
    addr = sock.addr;
    addr_len = sock.addr_len;
    buf = sock.buf;

    sock.fd = 0;
    sock.listening = false;
    std::memset(&sock.addr, 0, sock.addr_len);
    sock.addr_len = 0;
    sock.buf.clear();
    
    return *this;
}

Socket::~Socket() {
    if (fd > 2) {
        close(fd);
    }
}

bool Socket::try_listen(const std::string& port, int family, int socktype, int flags) { 
    if (fd != 0) return false;

    addrinfo hints, *ai, *p;
    int gai;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = flags;

    if ((gai = getaddrinfo(nullptr, port.c_str(), &hints, &ai)) != 0) {
        Logger::get().error(ERR_SOCKET_GET_ADDR_INFO_FAILED);
        Logger::get().error("server: socket: getaddrinfo: " + std::string(gai_strerror(gai)));
        return false;
    }

    [[maybe_unused]]
    auto yes = 1;

    for (p = ai; p != nullptr; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            Logger::get().log("server: socket: socket: " + std::string(strerror(errno)));
            continue;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            Logger::get().log("server: socket: bind: " + std::string(strerror(errno)));
            close(fd);
            continue;
        }

        break;
    }

    if (p == nullptr) {
        Logger::get().error(ERR_SOCKET_BIND_SOCKET_FAILED);
        Logger::get().error("server: failed to create and bind socket");
        return false;
    }

    freeaddrinfo(ai);

    if (evutil_make_socket_nonblocking(fd) == -1) {
        Logger::get().error(ERR_SOCKET_MAKE_NONBLOCKING_FAILED);
        Logger::get().error("server: socket: listen: evutil_make_socket_nonblocking: " + std::string(strerror(errno)));
        return false;
    }

    constexpr auto backlog = 10;

    if (listen(fd, backlog) == -1) {
        Logger::get().error(ERR_SOCKET_LISTEN_FAILED);
        Logger::get().error("server: socket: listen: " + std::string(strerror(errno)));
        return false;
    }

    listening = true;
    return true;
}

bool Socket::try_listen(const std::string& port) {
    return try_listen(port, AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
}

bool Socket::try_connect(const std::string& host, const std::string& port, bool nonblocking) {
    addrinfo hints, *ai, *p;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int gai;

    if ((gai = getaddrinfo(host == "" ? nullptr : host.c_str(), port.c_str(), &hints, &ai)) != 0) {
        Logger::get().error(ERR_SOCKET_CONNECT_GETADDRINFO_FAILED);
        Logger::get().error("server: socket: getaddrinfo: " + std::string(gai_strerror(gai)));
        return false;
    }

    for (p = ai; p; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("connect");
            continue;
        }

        break;
    }

    if (p == nullptr) {
        Logger::get().error(ERR_SOCKET_CONNECT_FAILED);
        Logger::get().error("server: socket: failed to connect");
        return false;
    }

    addr_len = p->ai_addrlen;
    std::memcpy(&addr, p->ai_addr, addr_len);
    freeaddrinfo(ai);

    if (nonblocking && evutil_make_socket_nonblocking(fd) == -1) {
        Logger::get().error(ERR_SOCKET_MAKE_NONBLOCKING_FAILED);
        Logger::get().error("server: socket: connect: evutil_make_socket_nonblocking: " + std::string(strerror(errno)));
        return false;
    }
    
    return true;
}

bool Socket::try_accept(Socket& socket) {
    if (!listening || socket.get_fd() > 0) return false;
    
    evutil_socket_t sock_fd;
    sockaddr_storage sock_addr;
    socklen_t sock_addr_len = sizeof sock_addr;
    std::memset(&sock_addr, 0, addr_len);

    if ((sock_fd = accept(fd, (sockaddr*)&sock_addr, &sock_addr_len)) == -1) {
        Logger::get().error(ERR_SOCKET_ACCEPT_CONN_FAILED);
        Logger::get().error("server: socket: accept: " + std::string(strerror(errno)));
        return false;
    }

    if (evutil_make_socket_nonblocking(sock_fd) == -1) {
        Logger::get().error(ERR_SOCKET_MAKE_NONBLOCKING_FAILED);
        Logger::get().error("server: socket: accept: evutil_make_socket_nonblocking: " + std::string(strerror(errno)));
        return false;
    }

    socket.fd = sock_fd;
    socket.addr = sock_addr;
    socket.addr_len = sock_addr_len;

    return true;
}

std::string Socket::get_host() const {
    char host[NI_MAXHOST];
    int gai = getnameinfo((sockaddr*)&addr, addr_len, host, sizeof host, nullptr, 0, 0);

    if (gai == 0) {
        return host;
    }

    Logger::get().error(ERR_SOCKET_GET_HOST_FAILED);
    Logger::get().error("server: socket: getnameinfo: " + std::string(gai_strerror(gai)));
    return "";
}

bool Socket::close_fd() {
    auto status = close(fd);

    fd = 0;
    listening = false;
    std::memset(&addr, 0, addr_len);

    return (status == 0);
}

std::pair<int32_t, uint32_t> Socket::try_recv(uint32_t (*write_cb) (char* dest, uint32_t n, void* data) noexcept, void* arg, uint32_t len) {
    std::lock_guard lock { recv_mux };
    return { buf.write(write_cb, len ? len : buf.space(), arg), buf.space() };
}

std::pair<int32_t, uint32_t> Socket::try_recv(uint32_t len) {
    return try_recv([] (char* dest, uint32_t n, void* data) noexcept -> uint32_t {
        auto socket = (Socket*)data;
        auto buffer = socket->buf;

        auto nbytes = recvfrom(socket->get_fd(), dest, n, 0, nullptr, 0);
        if (nbytes > 0) {
            return static_cast<uint32_t>(nbytes);
        }

        if (nbytes == -1) {
            if (!(errno & (EAGAIN|EWOULDBLOCK))) {
                Logger::get().error("server: socket: recvfrom: " + std::string(strerror(errno)));
            }
        }
        else {
            Logger::get().log("context: peer closed connection on sock " + std::to_string(socket->get_fd()));
            socket->close_fd();
        }

        return 0;
    }, this, len);
}

bool Socket::try_send(const std::string& data, bool terminate) {
    return try_send(std::vector<char>(data.c_str(), data.c_str() + data.size() + terminate), send);
}

bool Socket::try_send(const std::vector<char>& data) {
    return try_send(data, send);
}

bool Socket::try_send(const std::vector<char>& data, ssize_t send(int, const void *, size_t, int)) {
    if (!fd || listening) {
        Logger::get().error(ERR_SOCKET_INVALID_SEND_ATTEMPT);
        return false;
    }
            
    auto bytes = data.data();
    auto len = data.size();

    auto bytes_sent = 0;
    auto total = 0;
    
    {
        std::lock_guard lock { send_mux };

        while (total < len) {
            if ((bytes_sent = send(fd, bytes + total, len - total, 0)) == -1) {
                // @todo: send error
                Logger::get().error(ERR_SOCKET_SEND_FAILED);
                return false;
            }

            total += bytes_sent;
        }
    }

    return true;
}

std::vector<char> Socket::read_buffer(char delim) {
    if (buf.empty()) {
        return {};
    }

    std::lock_guard lock { buf_mux };
    auto bytes = buf.read_to(delim);

    /**
     * This might look odd: why not first search the buffer for the delimiter?
     * O(n) complexity for each is linear, so idea is to spare implementing a search fn
     * with an approach that works equally well for multi-byte delimiters.
     * 
     * A better question is if we should cancel the op when delim is not found?
     * 
     *  - The upside is that the caller does not have to worry about storing and piecing together
     * partial messages.
     * 
     *  - The downside is that we have to check for the delimiter in some way, but this approach is simple
     * and, as noted above, keeps the class interface small.
     */
    if (bytes.back() != delim) {
        buf.write(bytes);
        return {};      
    }

    bytes.pop_back();
    return bytes;
}

std::vector<char> Socket::read_buffer(std::string delim) {
    if (buf.empty()) {
        return {};
    }

    std::lock_guard lock { buf_mux };
    auto bytes = buf.read_to(delim);

    if (bytes.size() < delim.size() || std::string(bytes.end() - delim.size(), bytes.end()) != delim) {
        buf.write(bytes);
        return {};      
    }

    bytes.pop_back();
    return bytes;
}

std::string Socket::read_buffer() {
    auto data = read_buffer(0);
    return { data.begin(), data.end() };
}

std::vector<char> Socket::flush_buffer() {
    std::lock_guard lock { buf_mux };
    return buf.read();
}

void Socket::clear_buffer() {
    std::lock_guard lock { buf_mux };
    buf.clear();
}
