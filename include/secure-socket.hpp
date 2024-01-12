#ifndef INCLUDE_SECURE_SOCKET_H
#define INCLUDE_SECURE_SOCKET_H

#include <crypt/crypt.hpp>
#include <crypt/exchange.hpp>

namespace serv {

class Socket;

class SecureSocket {
    private:
        std::shared_ptr<Socket> sock;
        crpt::Crypt aes { "AES-256-CBC" };
        crpt::Exchange dh;
        bool is_secure;
        std::string shared_secret;
        std::string key;
        std::string iv;
    
    public:
        SecureSocket(std::shared_ptr<Socket>& sock);
        SecureSocket(SecureSocket& sock) = default;
        SecureSocket(SecureSocket&& sock) = default;
        ~SecureSocket() = default;
        SecureSocket& operator=(SecureSocket& sock) = default;
        SecureSocket& operator=(SecureSocket&& sock) = default;

        inline const Socket* const socket() const {
            return sock.get();
        }

        bool try_handshake();
};

}

#endif