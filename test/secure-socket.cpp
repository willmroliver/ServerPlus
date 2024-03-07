#include <boost/test/unit_test.hpp>
#include <chrono>
#include <memory>
#include <thread>
#include "socket.hpp"
#include "secure-socket.hpp"
#include "error-codes.hpp"
#include "helpers.hpp"
#include "client.hpp"
#include "peer-handshake.pb.h"

// BOOST_AUTO_TEST_CASE( test_secure_socket_handshake_init_fails ) {
//     clear_logger();

//     auto sock = std::make_shared<serv::Socket>();
//     serv::SecureSocket secure_sock { sock };

//     BOOST_ASSERT( !secure_sock.handshake_init() );
//     ASSERT_ERR_LOGGED( ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED );
// }

struct SecureSockFixture {
    serv::Socket listener;
    serv::SecureSocket sender;
    test::Client<1024> client;

    SecureSockFixture(): client { "8000" }, sender { std::make_shared<serv::Socket>() } {
        clear_logger();
        listener.try_listen("8000", AF_UNSPEC, SOCK_STREAM, AI_PASSIVE);
        client.try_connect();

        auto sock = std::make_shared<serv::Socket>();
        listener.try_accept(*sock);
        sender = serv::SecureSocket(sock);
    }

    ~SecureSockFixture() {
        client.try_close();
        listener.close_fd();
    }
};

// BOOST_FIXTURE_TEST_CASE( test_secure_socket_handshake_init, SecureSockFixture ) {
//     BOOST_ASSERT( sender.handshake_init() );
// }

void tiny_sleep() {
    // A little time buffer is necessary to ensure sender has received client handshake response.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ns);
}

// BOOST_FIXTURE_TEST_CASE( test_secure_socket_handshake_final, SecureSockFixture ) {
//     sender.handshake_init();
//     client.handshake_init();

//     tiny_sleep();
//     BOOST_ASSERT( sender.handshake_final() );

//     auto res = client.try_recv();
//     BOOST_ASSERT( res.size() == 1 && res[0] == 1 );
// }

// BOOST_FIXTURE_TEST_CASE( test_secure_socket_handshake_final_fails_on_close, SecureSockFixture ) {
//     sender.handshake_init();

//     client.try_close();

//     tiny_sleep();
//     BOOST_ASSERT( !sender.handshake_final() );
// }


// BOOST_FIXTURE_TEST_CASE( test_secure_socket_handshake_final_fails_on_bad_data, SecureSockFixture ) {
//     sender.handshake_init();

//     client.try_send("bad data");

//     tiny_sleep();
//     BOOST_ASSERT( !sender.handshake_final() );
//     ASSERT_ERR_LOGGED( ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED );
// }

// BOOST_FIXTURE_TEST_CASE( test_secure_socket_handshake_final_fails_on_bad_key, SecureSockFixture ) {
//     sender.handshake_init();

//     serv::proto::PeerHandshake peer_hs;

//     std::string bad_key = "bad data";
//     peer_hs.set_public_key(bad_key);
//     client.try_send(peer_hs.SerializeAsString());

//     tiny_sleep();
//     BOOST_ASSERT( !sender.handshake_final() );
//     ASSERT_ERR_LOGGED( ERR_SECURE_SOCKET_HANDSHAKE_FINAL_DERIVE_FAILED );
// }

// BOOST_AUTO_TEST_CASE( test_secure_socket_blocks_send_and_recv ) {
//     serv::Socket listener;
//     listener.try_listen("8000");

//     auto sender = std::make_shared<serv::Socket>();
//     listener.try_accept(*sender);

//     serv::SecureSocket sock { sender };
//     std::pair<int, bool> exp { -2, false };

//     BOOST_ASSERT( sock.try_recv() == exp );
//     BOOST_ASSERT( !sock.try_send("0123456789") );
// }

BOOST_AUTO_TEST_CASE( test_crypt_works ) {
    std::cout << "testing crypt works" << std::endl;
    crpt::Crypt aes { "AES-256-CBC" };

    std::vector<char> key(32, 'a');
    std::vector<char> iv(16, 'a');
    std::string plain_text = "12345678";

    auto [cipher_text, success] = aes.encrypt(std::vector<char>(plain_text.begin(), plain_text.end()), key, iv);

    BOOST_ASSERT( success );

    auto [decrypted, dec_suc] = aes.decrypt(cipher_text, key, iv);

    BOOST_ASSERT( dec_suc );
    BOOST_ASSERT( plain_text == std::string(decrypted.begin(), decrypted.end()));
}

BOOST_FIXTURE_TEST_CASE( test_secure_socket_data_sent_matches_data_recv, SecureSockFixture ) {
    sender.handshake_init();
    client.handshake_init();

    tiny_sleep();
    sender.handshake_final();
    client.handshake_final();

    auto data = "Hello, World!!!!!!!!!!!!";

    sender.clear_buffer();
    client.try_send(data);

    tiny_sleep();
    auto [len, full] = sender.try_recv();
    BOOST_ASSERT( len > -1 );

    auto recvd = sender.flush_buffer();

    std::cout << std::string(recvd.begin(), recvd.end()) << std::endl;

    BOOST_ASSERT( std::string(recvd.begin(), recvd.end()) == data);
}