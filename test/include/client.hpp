#ifndef INCLUDE_CLIENT_H
#define INCLUDE_CLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <event2/event.h>
#include <unistd.h>

namespace test {

template <unsigned BUF_SIZE>
class Client {
    private:
        std::string port;
        evutil_socket_t fd;

    public:
        Client(): port { "3993" }, fd { 0 } {};
        Client(std::string port): port { port }, fd { 0 } {};
        ~Client() {
            if (!fd) return;
            if (close(fd) == -1 && errno != EBADF) perror("close");
        };

        bool try_connect() {
            addrinfo hints, *ai, *p;
            memset(&hints, 0, sizeof hints);

            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            int gai;

            if ((gai = getaddrinfo(nullptr, port.c_str(), &hints, &ai)) != 0) {
                std::cerr << "getaddrinfo: " << gai_strerror(gai) << std::endl;
                exit(EXIT_FAILURE);
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
                std::cerr << "test client: failed to connect" << std::endl;
                exit(EXIT_FAILURE);
            }

            char host[NI_MAXHOST];

            if ((gai = getnameinfo(p->ai_addr, p->ai_addrlen, host, NI_MAXHOST, nullptr, 0, NI_NUMERICSERV)) != 0) {
                std::cerr << "getnameinfo: " << gai_strerror(gai) << std::endl;
            }

            freeaddrinfo(ai);

            return true;
        }

        bool try_close() {
            if (!fd) return true;

            if (close(fd) == -1 && errno != EBADF) {
                perror("close");
                return false;
            }

            fd = 0;
            return true;
        }

        bool try_send(const std::string req, size_t len = 0) const {
            if (!fd) {
                return false;
            }

            if (len == 0) {
                len = req.size() + 1;
            }
            
            const char* data = req.c_str();

            auto bytes_sent = 0;
            auto total = 0;

            while (total < len) {
                if ((bytes_sent = send(fd, data + total, len - total, 0)) == -1) {
                    perror("send");
                    return false;
                }
                total += bytes_sent;
            }

            return true;
        }

        std::string try_recv() const {
            if (!fd) return "";

            char buf[BUF_SIZE];
            unsigned len;
            
            if ((len = recvfrom(fd, buf, BUF_SIZE, 0, nullptr, 0)) == -1) {
                perror("recvfrom");
                return "";
            } 
            if (len == 0) {
                std::cout << "client: peer closed connection" << std::endl;
                return "";
            }

            return { buf, len };
        }

        const evutil_socket_t get_fd() const {
            return fd;
        }
};

};

#endif