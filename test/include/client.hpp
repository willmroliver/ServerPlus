#ifndef INCLUDE_CLIENT_H
#define INCLUDE_CLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <event2/event.h>
#include <unistd.h>
#include <crypt/exchange.hpp>
#include <crypt/crypt.hpp>
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"

namespace test {

template <unsigned BUF_SIZE>
class Client {
    private:
        std::string port;
        evutil_socket_t fd;
        std::vector<char> key;
        std::vector<char> iv;
        crpt::Crypt aes { "AES-256-CBC" };
        crpt::Exchange dh { "ffdhe2048" };
        bool secure = false;

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

        /**
         * @brief Convenience function to initialise a correct peer handshake response.
         * An integration test for the sequence of calls made by this function exists in test/server.cpp.
         */
        bool handshake_init() {
            serv::proto::HostHandshake host_hs;

            if (!host_hs.ParseFromString(try_recv())) {
                return false;
            }
            
            dh = crpt::Exchange("ffdhe2048");
            iv = std::vector<char>(host_hs.iv().begin(), host_hs.iv().end());

            crpt::PublicKeyDER host_pk;
            auto host_pk_str = host_hs.public_key();
            host_pk.from_vector({ host_pk_str.begin(), host_pk_str.end() });

            if (!dh.derive_secret(host_pk)) {
                return false;
            }

            serv::proto::PeerHandshake peer_hs;
            auto peer_pk = dh.get_public_key().to_vector();
            peer_hs.set_public_key({ peer_pk.begin(), peer_pk.end() });

            if (!try_send(peer_hs.SerializeAsString())) {
                return false;
            }

            return true;
        }

        /**
         * @brief Convenience function to finalise a correct peer handshake response.
         * An integration test for the sequence of calls made by this function exists in test/server.cpp.
         */
        bool handshake_final() {
            auto res = try_recv();
            if (res.size() != 1 || res[0] != 1) {
                return false;
            }

            auto [secret_hash, success] = crpt::Crypt::hash(dh.get_secret());
            if (!success) {
                return false;
            }

            key = secret_hash;
            secure = true;

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

        bool try_send(const std::string req, size_t len = 0) {
            if (!fd) {
                return false;
            }

            if (len == 0) {
                len = req.size() + 1;
            }
            
            std::vector<char> cipher_text_cpy;
            const char* data;

            if (secure) {
                auto [cipher_text, success] = aes.encrypt(std::vector<char>(req.begin(), req.end()), key, iv);

                if (!success) {
                    return false;
                }

                cipher_text_cpy = cipher_text;
                data = cipher_text_cpy.data();
                len = cipher_text.size();
            }
            else {
                data = req.c_str();
            }

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

        std::string try_recv() {
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

            if (secure) {
                auto cipher_text = std::vector<char>(buf, buf + len);
                auto [plain_text, success] = aes.decrypt(std::vector<char>(buf, buf + len), key, iv);
                return std::string(plain_text.begin(), plain_text.end());
            }

            return { buf, len };
        }

        const evutil_socket_t get_fd() const {
            return fd;
        }
};

};

#endif