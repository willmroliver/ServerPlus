#include <iostream>
#include <string>
#include <memory>

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

Socket::~Socket() {
    close(fd);
}

bool Socket::try_listen(std::string port, int family, int socktype, int flags) { 

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
        Logger::get().error("server: socket: evutil_make_socket_nonblocking: " + std::string(strerror(errno)));
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

bool Socket::try_listen(std::string port) {
    return try_listen(port, AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
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
        Logger::get().error("server: socket: evutil_make_socket_nonblocking: " + std::string(strerror(errno)));
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

std::pair<int, bool> Socket::try_recv(int (*write_cb) (char* dest, unsigned n, void* data), void* arg) {
    auto res = buf.write(write_cb, buf.bytes_free(), arg);

    if (!res.second) {
        Logger::get().error(ERR_SOCKET_RECV_FAILED);
    }
    
    return res;
}

std::pair<int, bool> Socket::try_recv() {
    return try_recv([] (char* dest, unsigned n, void* data) {
        auto socket = (Socket*)data;
        auto buffer = socket->buf;

        auto nbytes = recvfrom(socket->get_fd(), dest, n, 0, nullptr, 0);

        if (nbytes <= 0) {
            if (nbytes == -1) {
                Logger::get().error("server: socket: recvfrom: " + std::string(strerror(errno)));
            }
            else {
                Logger::get().log("context: peer closed connection on sock " + std::to_string(socket->get_fd()));
            }
            
            socket->close_fd();
            return 0;
        }

        return static_cast<int>(nbytes);
    }, this);
}

bool Socket::try_send(const std::string& data) {
    return try_send(std::vector<char>(data.begin(), data.end()), send);
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

    while (total < len) {
        if ((bytes_sent = send(fd, bytes + total, len - total, 0)) == -1) {
            // send error
            Logger::get().error(ERR_SOCKET_SEND_FAILED);
            return false;
        }

        total += bytes_sent;
    }

    return true;
}

std::vector<char> Socket::read_buffer(char delim) {
    if (buf.contains(delim)) {
        auto [res, found] = buf.read_to(delim);
        return res;
    }

    return {};
}

std::string Socket::read_buffer() {
    auto data = read_buffer(0);
    return { data.begin(), data.end() };
}

std::vector<char> Socket::flush_buffer() {
    return buf.read();
}

void Socket::clear_buffer() {
    buf.clear();
}
