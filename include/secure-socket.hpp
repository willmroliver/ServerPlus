#ifndef INCLUDE_SECURE_SOCKET_H
#define INCLUDE_SECURE_SOCKET_H

#include <crypt/crypt.hpp>
#include <crypt/exchange.hpp>
#include "socket.hpp"

namespace serv {

class SecureSocket : public Socket {
    private:
        crpt::Crypt aes { "AES-256-CBC" };
        crpt::Exchange dh { "ffdhe2048" };
        std::vector<char> shared_secret;
        std::vector<char> key;
        std::vector<char> iv;
        bool is_secure = false;
    
    public:
        SecureSocket();
        SecureSocket(SecureSocket& sock);
        SecureSocket(SecureSocket&& sock);
        SecureSocket& operator=(SecureSocket& sock);
        SecureSocket& operator=(SecureSocket&& sock);

        /**
         * @brief Initialize a handshake from the host, passing the public key & IV to the peer.
         * 
         * @return bool The success or failure of the initialization attempt.
         */
        bool handshake_init();

        /**
         * @brief Retrieve the public key from the peer and attempt to derive a shared secret & 256-bit key.
         * 
         * @return bool The success or failure of the retrieval & derivation attempt.
         */
        bool handshake_final();

        /**
         * @brief If secure, retrieves and decrypts sock data. See Socket::try_rev()
         * 
         * @return std::pair<int, bool> The number of bytes read (-2 indicates socket is not secure, -1 indicates error) and whether the buffer has space remaining.
         */
        std::pair<int, bool> try_recv();

        /**
         * @brief If secure, encrypts and sends sock data. See Socket::try_send()
         * 
         * @param data The data to encrypt and send.
         * @return bool The success or failure of the attempt to ancrypt and send.
         */
        bool try_send(std::string data);
};

}

#endif