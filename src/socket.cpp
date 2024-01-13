#include "socket.hpp"
#include <iostream>
#include <string>
#include <memory>

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
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return false;
    }

    [[maybe_unused]]
    auto yes = 1;

    for (p = ai; p != nullptr; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind");
            close(fd);
            continue;
        }

        break;
    }

    if (p == nullptr) {
        fprintf(stderr, "server: failed to connect");
        return false;
    }

    freeaddrinfo(ai);

    if (evutil_make_socket_nonblocking(fd) == -1) {
        perror("evutil_make_socket_nonblocking");
        return false;
    }

    constexpr auto backlog = 10;

    if (listen(fd, backlog) == -1) {
        perror("listen");
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
        perror("accept");
        return false;
    }

    if (evutil_make_socket_nonblocking(sock_fd) == -1) {
        perror("evutil_make_socket_nonblocking");
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

    std::cerr << "getnameinfo: " << gai_strerror(gai) << std::endl;
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
    return buf.write(write_cb, buf.bytes_free(), arg);
}

std::pair<int, bool> Socket::try_recv() {
    return try_recv([] (char* dest, unsigned n, void* data) {
        auto socket = (Socket*)data;
        auto buffer = socket->buf;

        auto nbytes = recvfrom(socket->get_fd(), dest, n, 0, nullptr, 0);

        if (nbytes <= 0) {
            if (nbytes == -1) {
                perror("context: recvfrom");
            }
            else {
                std::cout << "context: peer closed connection on sock " << socket->get_fd() << std::endl;
            }
            
            socket->close_fd();
            return 0;
        }

        return static_cast<int>(nbytes);
    }, this);
}

bool Socket::try_send(std::string data) {
    if (!fd || listening) return false;
            
    auto bytes = data.c_str();
    auto len = data.size();

    auto bytes_sent = 0;
    auto total = 0;

    while (total < len) {
        if ((bytes_sent = send(fd, bytes + total, len - total, 0)) == -1) {
            // send error
            return false;
        }

        total += bytes_sent;
    }

    return true;
}

bool Socket::try_send(std::vector<char> data) {
    if (!fd || listening) return false;
            
    auto bytes = data.data();
    auto len = data.size();

    auto bytes_sent = 0;
    auto total = 0;

    while (total < len) {
        if ((bytes_sent = send(fd, bytes + total, len - total, 0)) == -1) {
            // send error
            return false;
        }

        total += bytes_sent;
    }

    return true;
}

std::vector<char> Socket::retrieve_data(char delim) {
    if (buf.contains(delim)) {
        auto [res, found] = buf.read_to(delim);
        return res;
    }

    return {};
}

std::string Socket::retrieve_message() {
    auto data = retrieve_data(0);
    return { data.begin(), data.end() };
}

std::vector<char> Socket::flush_buffer() {
    return buf.read();
}

void Socket::clear_buffer() {
    buf.clear();
}
