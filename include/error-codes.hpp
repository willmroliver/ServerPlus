#ifndef INCLUDE_ERROR_CODES_H
#define INCLUDE_ERROR_CODES_H

#include <unordered_map>
#include <string>

/**
 * Error codes follow the following format: CCNNN
 * 
 * - CC represents a unique class. All error codes prefixed with the same two digits strictly belong to a single class.
 * 
 * - NNN is a unique identifier within each class.
 * - Usually it corresponds to the order of appearance in code, though this is not a hard rule.
 * 
 * Error codes not belonging to a specific class can be expected to simply drop CC, so are 3-digits: NNN.
 * 
 */

// Socket
constexpr int ERR_SOCKET_GET_ADDR_INFO_FAILED = 11001;
constexpr int ERR_SOCKET_BIND_SOCKET_FAILED = 11002;
constexpr int ERR_SOCKET_LISTEN_FAILED = 11003;
constexpr int ERR_SOCKET_MAKE_NONBLOCKING_FAILED = 11004;
constexpr int ERR_SOCKET_ACCEPT_CONN_FAILED = 11005;
constexpr int ERR_SOCKET_GET_HOST_FAILED = 11006;
constexpr int ERR_SOCKET_BUFFER_FULL = 11007;
constexpr int ERR_SOCKET_RECV_FAILED = 11008;
constexpr int ERR_SOCKET_INVALID_SEND_ATTEMPT = 11009;
constexpr int ERR_SOCKET_SEND_FAILED = 11010;

// SecureSocket
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED = 12001;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED = 12002;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_DERIVE_FAILED = 12003;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_HASH_FAILED = 12004;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_FAILED = 12005;
constexpr int ERR_SECURE_SOCKET_RECV_FAILED = 12006;
constexpr int ERR_SECURE_SOCKET_SEND_FAILED = 12007;

// Context
constexpr int ERR_CONTEXT_BUFFER_FULL = 13001;
constexpr int ERR_CONTEXT_HANDLE_READ_FAILED = 13002;

// Server
constexpr int ERR_SERVER_ACCEPT_CONN_FAILED = 13003;

static std::unordered_map<int, std::string> error_messages = {
    // Socket
    { ERR_SOCKET_GET_ADDR_INFO_FAILED, "Socket: failed to find address information." },
    { ERR_SOCKET_BIND_SOCKET_FAILED, "Socket: failed to open and bind socket." },
    { ERR_SOCKET_LISTEN_FAILED, "Socket: failed to start listening on bound socket." },
    { ERR_SOCKET_MAKE_NONBLOCKING_FAILED, "Socket: failed to make socket non-blocking." },
    { ERR_SOCKET_ACCEPT_CONN_FAILED, "Socket: failed to accept incoming connection." },
    { ERR_SOCKET_GET_HOST_FAILED, "Socket: failed to get host information." },
    { ERR_SOCKET_BUFFER_FULL, "Socket: incoming data exceeded buffer size." },
    { ERR_SOCKET_RECV_FAILED, "Socket: failed to receive incoming data." },
    { ERR_SOCKET_INVALID_SEND_ATTEMPT, "Socket: attempted to send on a closed or listening socket." },
    { ERR_SOCKET_SEND_FAILED, "Socket: failed to send data." },

    // SecureSocket
    { ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED, "SecureSocket: failed to initialize handshake." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED, "SecureSocket: failed to parse handshake response." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_DERIVE_FAILED, "SecureSocket: failed to derive shared secret." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_HASH_FAILED, "SecureSocket: failed to generate symmetric key." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_FAILED, "SecureSocket: failed to finalize handshake." },
    { ERR_SECURE_SOCKET_RECV_FAILED, "SecureSocket: failed to receive incoming data" },
    { ERR_SECURE_SOCKET_SEND_FAILED, "SecureSocket: failed to send data." },

    // Context
    { ERR_CONTEXT_BUFFER_FULL, "Context: incoming data exceeded context buffer size" },
    { ERR_CONTEXT_HANDLE_READ_FAILED, "Context: failed to handle socket request" },

    // Server
    { ERR_SERVER_ACCEPT_CONN_FAILED, "Server: failed to accept incoming connection" },
};

#endif