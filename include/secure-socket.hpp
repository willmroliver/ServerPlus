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
        SecureSocket() = default;
        SecureSocket(Socket&& s);
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
         * @brief Accepts a handshake initializtion and returns its own public key to the requestor.
         *
         * @return bool The success or failure of the accept attempt.
         */
        bool handshake_accept();

        /**
         * @brief Retrieve the public key from the peer and attempt to derive a shared secret & 256-bit key.
         * 
         * @return bool The success or failure of the retrieval & derivation attempt.
         */
        bool handshake_final();

        /**
         * @brief Confirms that a handshake has been completed by the host and derives the shared secret + key.
         * 
         * @return bool The success or failure of the derivation attempt.
         */
        bool handshake_confirm();

        /**
         * @brief If secure, retrieves and decrypts sock data. See Socket::try_rev()
         * 
         * @return std::pair<int, bool> The number of bytes read (-2 indicates socket is not secure, -1 indicates error) and the remaining buffer space.
         */
        std::pair<int32_t, uint32_t> try_recv();

        /**
         * @brief If secure, encrypts and sends sock data. See Socket::try_send()
         * 
         * @param data The data to encrypt and send.
         * @param terminate Whether to include the null-terminator, default is true.
         * @return bool The success or failure of the attempt to ancrypt and send.
         */
        bool try_send(std::string data, bool terminate=true);
};

}

#endif