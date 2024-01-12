#include <crypt/util.hpp>
#include "secure-socket.hpp"
#include "socket.hpp"
#include "host-handshake.pb.h"

using namespace serv;

SecureSocket::SecureSocket(std::shared_ptr<Socket>& sock): 
    sock { sock }
{}

bool SecureSocket::try_handshake() {
    shared_secret = "";
    key = "";

    iv = crpt::util::rand_bytes(16);
    auto host_pk = dh.get_public_key();

    serv::proto::HostHandshake handshake;
    handshake.set_public_key(host_pk.to_string());
    handshake.set_iv(iv);

    sock->try_send(handshake.SerializeAsString());
    auto [res, success] = sock->try_recv();

    if (!success) {
        return false;
    }

    return true;
}