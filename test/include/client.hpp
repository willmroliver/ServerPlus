#ifndef INCLUDE_CLIENT_H
#define INCLUDE_CLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <event2/event.h>
#include <unistd.h>
#include <crypt/exchange.hpp>
#include <crypt/crypt.hpp>
#include "secure-socket.hpp"
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"

namespace test {

const unsigned BUF_SIZE = 1024;

class Client {
    private:
        std::string port;
        evutil_socket_t fd;
        std::vector<char> key;
        std::vector<char> iv;
        crpt::Crypt aes { "AES-256-CBC" };
        crpt::Exchange dh { "ffdhe2048" };
        bool secure = false;
        serv::Socket sock;
        serv::SecureSocket ssock;
        
    public:
        Client(): port { "3993" }, fd { 0 } {};
        Client(std::string port): port { port }, fd { 0 } {};
        ~Client() {
            if (!fd) return;
            if (close(fd) == -1 && errno != EBADF) perror("close");
        };
        Client& operator=(Client&& client) {
            port = client.port;
            fd = client.fd;
            key = client.key;
            iv = client.iv;
            aes = crpt::Crypt { "AES-256-CBC" };
            dh = crpt::Exchange { "ffdhe2048" };
            secure = client.secure;

            client.port = "";
            client.fd = 0;
            client.key.clear();
            client.iv.clear();
            client.secure = false;

            return *this;
        }

        bool try_connect() {
            return sock.try_connect("", port, false);
        }

        bool handshake_init() {
            secure = false;
            ssock = serv::SecureSocket(std::move(sock));
            return ssock.handshake_accept();
        }

        bool handshake_final() {
            if (ssock.handshake_confirm()) {
                secure = true;
                return true;
            }
            
            return false;
        }

        bool try_close() {
            return sock.close_fd() || ssock.close_fd();
        }

        bool try_send(const std::string req, size_t len = 0) {
            if (secure) {
                return ssock.try_send(len ? req.substr(0, len) : req);
            } else {
                return sock.try_send(len ? req.substr(0, len) : req);
            }
        }
        
        std::string try_recv() {
            if (secure) {
                ssock.try_recv();
                return ssock.read_buffer();
            } else {
                sock.try_recv();
                return sock.read_buffer();
            }
        }

        const evutil_socket_t get_fd() const {
            return fd;
        }
};

};

#endif