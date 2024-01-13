#include <crypt/util.hpp>
#include <crypt/public-key-der.hpp>
#include "secure-socket.hpp"
#include "socket.hpp"
#include "socket.hpp"
#include "host-handshake.pb.h"
#include "peer-handshake.pb.h"

using namespace serv;

SecureSocket::SecureSocket(std::shared_ptr<Socket>& sock): 
    sock { sock }
{}

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

    return sock->try_send(data);
}

bool SecureSocket::handshake_final() {
    auto [nbytes, full] = sock->try_recv();

    if (nbytes < 1) {
        if (nbytes == -1) {
            // error
        }

        return false;
    }

    serv::proto::PeerHandshake handshake;

    auto bytes = sock->flush_buffer();
    std::string data { bytes.begin(), bytes.end() - 1 };    // - 1 to exclude the null delimiter

    if (!handshake.ParseFromString(data)) {
        // error
        return false;
    }
    
    auto pk_str = handshake.public_key();

    crpt::PublicKeyDER peer_pk;
    peer_pk.from_vector({ pk_str.begin(), pk_str.end() });

    if (!dh.derive_secret(peer_pk)) {
        // error
        return false;
    }

    shared_secret = dh.get_secret();
    auto [secret_hash, success] = crpt::Crypt::hash(shared_secret);

    if (!success) {
        // error
        return false;
    }

    key = secret_hash;
    is_secure = true;

    // Let's send a single byte, value 1, to indicate the success of the handshake.
    return sock->try_send(std::vector<char> { 1 });
}

std::pair<int, bool> SecureSocket::try_recv() {
    if (!is_secure) {
        sock->try_recv();
        sock->clear_buffer();
        return { -1, false };
    }

    auto sock_recv = sock->try_recv();

    if (sock_recv.first < 1) {
        // error
        return sock_recv;
    }

    auto cipher_text = sock->flush_buffer();
    auto [plain_text, success] = aes.decrypt(cipher_text, key, iv);

    if (!(success && buf.write(plain_text))) {
        // error
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
        // error
        return false;
    }

    return sock->try_send(plain_text);
}

std::vector<char> SecureSocket::retrieve_data(char delim) {
    if (buf.contains(delim)) {
        auto [res, found] = buf.read_to(delim);
        return res;
    }

    return {};
}

std::string SecureSocket::retrieve_message()  {
    auto data = retrieve_data(0);
    return { data.begin(), data.end() };
}

void SecureSocket::clear_buffer() {
    sock->clear_buffer();
    buf.clear();
}