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

SecureSocket::SecureSocket(Socket&& sock):
    Socket { std::move(sock) }
{}

SecureSocket::SecureSocket(SecureSocket& sock): 
    Socket { sock },
    is_secure { false }
{}

SecureSocket::SecureSocket(SecureSocket&& sock): 
    Socket { std::move(sock) },
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

    shared_secret = sock.shared_secret;
    key = sock.key;
    iv = sock.iv;
    is_secure = sock.is_secure;

    return *this;
}

SecureSocket& SecureSocket::operator=(SecureSocket&& sock) {
    Socket::operator=(std::move(sock));

    shared_secret = sock.shared_secret;
    key = sock.key;
    iv = sock.iv;
    is_secure = sock.is_secure;

    sock.shared_secret = {};
    sock.key = {};
    sock.iv = {};
    sock.is_secure = false;

    return *this;
}

bool SecureSocket::handshake_init() {
    is_secure = false;
    shared_secret.clear();
    key.clear();

    iv = crpt::util::rand_bytes(16);
    auto host_pk = dh.get_public_key().to_vector();

    serv::proto::HostHandshake host_hs;
    host_hs.set_public_key({ host_pk.begin(), host_pk.end() });
    host_hs.set_iv({ iv.begin(), iv.end() });

    if (!Socket::try_send(host_hs.SerializeAsString())) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED);
        return false;
    }

    return true;
}

bool SecureSocket::handshake_accept() {
    is_secure = false;
    shared_secret.clear();
    key.clear();

    auto [nbytes, _] = Socket::try_recv();
    if (nbytes < 1) {
        return false;
    }

    auto bytes = Socket::flush_buffer();
    std::string data { bytes.begin(), bytes.end() - 1 };
    
    proto::HostHandshake host_hs;
    if (!host_hs.ParseFromString(data)) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_PARSE_FAILED);   
        return false;
    }
    
    iv = std::vector<char>(host_hs.iv().begin(), host_hs.iv().end());

    crpt::PublicKeyDER host_pk;
    auto host_pk_str = host_hs.public_key();
    host_pk.from_vector({ host_pk_str.begin(), host_pk_str.end() });

    if (!dh.derive_secret(host_pk)) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_DERIVE_FAILED);
        return false;
    }

    proto::PeerHandshake peer_hs;
    auto peer_pk = dh.get_public_key().to_vector();
    peer_hs.set_public_key({ peer_pk.begin(), peer_pk.end() });

    if (!Socket::try_send(peer_hs.SerializeAsString())) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_SEND_FAILED);
        return false;
    }

    return true;
}

bool SecureSocket::handshake_final() {
    auto [nbytes, _] = Socket::try_recv();
    if (nbytes < 1) {
        return false;
    }

    auto bytes = Socket::flush_buffer();
    std::string data { bytes.begin(), bytes.end() - 1 };    // - 1 to exclude the null delimiter
    
    serv::proto::PeerHandshake peer_hs;
    if (!peer_hs.ParseFromString(data)) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED);
        return false;
    }
    
    auto pk_str = peer_hs.public_key();

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

    // Let's send a single byte, value 1 (still null-terminated), to indicate the success of the handshake.
    if (!Socket::try_send(std::vector<char> { 1, 0 })) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_FINAL_FAILED);
        return false;
    }

    return true;
}

bool SecureSocket::handshake_confirm() {
    auto [nbytes, _] = Socket::try_recv(2); // { 1, 0 }

    if (nbytes != 2) {
        return false;
    }

    auto res = read_buffer(0);
    if (res.front() != 1) {
        return false;
    }

    shared_secret = dh.get_secret();
    auto [secret_hash, success] = crpt::Crypt::hash(shared_secret);

    if (!success) {
        Logger::get().error(ERR_SECURE_SOCKET_HANDSHAKE_CONFIRM_DERIVE_FAILED);
        return false;
    }

    key = secret_hash;
    is_secure = true;

    return true;
}

std::pair<int32_t, uint32_t> SecureSocket::try_recv() {
    if (!is_secure) {
        Socket::try_recv();
        Socket::clear_buffer();
        return { -2, false };
    }

    int offset = buf.size();
    
    const auto sock_recv = Socket::try_recv();

    if (sock_recv.first < 1) {
        return sock_recv;
    }

    auto cipher_text = buf.read_from(offset);
    auto [plain_text, success] = aes.decrypt(cipher_text, key, iv);

    if (!(success && buf.write(plain_text))) {
        Logger::get().error(ERR_SECURE_SOCKET_RECV_FAILED);
        return { -1, sock_recv.second };
    }
    
    return { plain_text.size(), sock_recv.second };
}

bool SecureSocket::try_send(std::string data, bool terminate) {
    if (!is_secure) {
        return false;
    }

    std::vector<char> plain_text { data.c_str(), data.c_str() + data.size() + terminate };
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