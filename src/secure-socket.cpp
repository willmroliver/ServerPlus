#include <crypt/util.hpp>
#include <crypt/public-key-der.hpp>
#include <crypt/error.hpp>
#include <event2/util.h>
#include "secure-socket.hpp"
#include "socket.hpp"
#include "socket.hpp"
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"
#include "logger.hpp"
#include "error-codes.hpp"

using namespace serv;

SecureSocket::SecureSocket(): 
    Socket {}
{}

SecureSocket::SecureSocket(SecureSocket& sock): 
    Socket { sock },
    aes { "AES-256-CBC" },
    dh { "ffdhe2048" },
    shared_secret { sock.shared_secret },
    key { sock.key },
    iv { sock.iv },
    is_secure { sock.is_secure }
{}

SecureSocket::SecureSocket(SecureSocket&& sock): 
    Socket { std::move(sock) },
    aes { "AES-256-CBC" },
    dh { "ffdhe2048" },
    shared_secret { sock.shared_secret },
    key { sock.key },
    iv { sock.iv },
    is_secure { sock.is_secure }
{
    sock.shared_secret = {};
    sock.key = {};
    sock.iv = {};
    sock.is_secure = false;
}

SecureSocket& SecureSocket::operator=(SecureSocket& sock) {
    Socket::operator=(sock);
    return *this;
}

SecureSocket& SecureSocket::operator=(SecureSocket&& sock) {
    Socket::operator=(std::move(sock));
    return *this;
}

bool SecureSocket::handshake_init() {
    is_secure = false;
    shared_secret.clear();
    key.clear();

    iv = crpt::util::rand_bytes(16);
    auto host_pk = dh.get_public_key().to_vector();

    serv::proto::HostHandshake handshake;
    handshake.set_public_key({ host_pk.begin(), host_pk.end() });
    handshake.set_iv({ iv.begin(), iv.end() });

    auto data = handshake.SerializeAsString();

    if (!Socket::try_send(data)) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED);
        return false;
    }

    return true;
}

bool SecureSocket::handshake_final() {
    auto [nbytes, full] = Socket::try_recv();

    if (nbytes < 1) {
        return false;
    }

    serv::proto::PeerHandshake handshake;

    auto bytes = Socket::flush_buffer();
    std::string data { bytes.begin(), bytes.end() - 1 };    // - 1 to exclude the null delimiter

    if (!handshake.ParseFromString(data)) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED);
        return false;
    }
    
    auto pk_str = handshake.public_key();

    crpt::PublicKeyDER peer_pk;
    peer_pk.from_vector({ pk_str.begin(), pk_str.end() });

    if (!dh.derive_secret(peer_pk)) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_FINAL_DERIVE_FAILED);
        return false;
    }

    shared_secret = dh.get_secret();
    auto [secret_hash, success] = crpt::Crypt::hash(shared_secret);

    if (!success) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_FINAL_HASH_FAILED);
        return false;
    }

    key = secret_hash;
    is_secure = true;

    // Let's send a single byte, value 1, to indicate the success of the handshake.
    if (!Socket::try_send(std::vector<char> { 1 })) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_FINAL_FAILED);
        return false;
    }

    return true;
}

std::pair<int, bool> SecureSocket::try_recv() {
    if (!is_secure) {
        Socket::try_recv();
        Socket::clear_buffer();
        return { -2, false };
    }

    auto sock_recv = Socket::try_recv();

    if (!sock_recv.second) {
        Logger::get().error(ERR_SECURE_SOCKET_RECV_FAILED);
        return sock_recv;
    }

    if (sock_recv.first < 1) {
        return sock_recv;
    }

    auto cipher_text = Socket::flush_buffer();
    auto [plain_text, success] = aes.decrypt(cipher_text, key, iv);

    if (!(success && buf.write(plain_text))) {
        Logger::get().error(ERR_SECURE_SOCKET_RECV_FAILED);
        return { -1, sock_recv.second };
    }
    
    return { plain_text.size(), !buf.bytes_free() };
}

bool SecureSocket::try_send(std::string data) {
    if (!is_secure) {
        return false;
    }

    std::vector<char> plain_text { data.begin(), data.end() };
    auto [cipher_text, success] = aes.encrypt(plain_text, key, iv);

    if (!success) {
        Logger::get().error(ERR_SECURE_SOCKET_SEND_FAILED);
        return false;
    }

    if (!Socket::try_send(cipher_text)) {
        Logger::get().error(ERR_SECURE_SOCKET_SEND_FAILED);
        return false;
    }

    return true;
}

std::vector<char> SecureSocket::read_buffer(char delim) {
    if (buf.contains(delim)) {
        auto [res, found] = buf.read_to(delim);
        return res;
    }

    return {};
}

std::string SecureSocket::read_buffer()  {
    auto data = read_buffer(0);
    return { data.begin(), data.end() };
}

std::vector<char> SecureSocket::flush_buffer() {
    return buf.read();
}

void SecureSocket::clear_buffer() {
    Socket::clear_buffer();
    buf.clear();
}