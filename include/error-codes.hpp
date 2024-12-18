#ifndef INCLUDE_ERROR_CODES_H
#define INCLUDE_ERROR_CODES_H

#include <unordered_map>
#include <string>

/**
 * Internal error codes follow the following format: CCNNN
 * 
 * - CC represents a unique class. All error codes prefixed with the same two digits strictly belong to a single class.
 * 
 * - NNN is a unique identifier within each class.
 * - Usually it corresponds to the order of appearance in code, though this is not a hard rule.
 * 
 * Internal error codes not belonging to a specific class should be prepended with 10, thus all following the format 10NNN.
 * 
 * It follows from this formatting that error codes built on top of this library, 
 * such as for use in user-defined handlers, ought to consider numbers 10001 - 99999 reserved.
 */

// General
constexpr int ERR_UNKNOWN = 10001;  // In practice, we should always endeavour to use a more informative error code than this...

// Socket
constexpr int ERR_SOCKET_GET_ADDR_INFO_FAILED = 11001;
constexpr int ERR_SOCKET_BIND_SOCKET_FAILED = 11002;
constexpr int ERR_SOCKET_LISTEN_FAILED = 11003;
constexpr int ERR_SOCKET_CONNECT_GETADDRINFO_FAILED = 11004;
constexpr int ERR_SOCKET_CONNECT_FAILED = 1105;
constexpr int ERR_SOCKET_MAKE_NONBLOCKING_FAILED = 11006;
constexpr int ERR_SOCKET_ACCEPT_CONN_FAILED = 11007;
constexpr int ERR_SOCKET_GET_HOST_FAILED = 11008;
constexpr int ERR_SOCKET_BUFFER_FULL = 11009;
constexpr int ERR_SOCKET_RECV_FAILED = 11010;
constexpr int ERR_SOCKET_INVALID_SEND_ATTEMPT = 11011;
constexpr int ERR_SOCKET_SEND_FAILED = 11012;

// SecureSocket
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED = 12001;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_PARSE_FAILED = 12002;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_DERIVE_FAILED = 12003;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_SEND_FAILED = 12004;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED = 12005;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_DERIVE_FAILED = 12006;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_HASH_FAILED = 12007;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_FINAL_FAILED = 12008;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_CONFIRM_DERIVE_FAILED = 12009;
constexpr int ERR_SECURE_SOCKET_HANDSHAKE_CONFIRM_SEND_FAILED = 12010;
constexpr int ERR_SECURE_SOCKET_RECV_FAILED = 12011;
constexpr int ERR_SECURE_SOCKET_SEND_FAILED = 12012;

// Context
constexpr int ERR_CONTEXT_BUFFER_FULL = 13001;
constexpr int ERR_CONTEXT_HANDLE_REQUEST_FAILED = 13002;
constexpr int ERR_CONTEXT_HANDLE_READ_FAILED = 13003;
constexpr int ERR_CONTEXT_DO_ERROR_FAILED = 13004;
constexpr int ERR_CONTEXT_PING_FAILED = 13005;
constexpr int ERR_CONTEXT_SEND_MESSAGE_FAILED = 13006;

// Server
constexpr int ERR_SERVER_ACCEPT_CONN_FAILED = 14001;

// ThreadPool
constexpr int ERR_THREAD_POOL_THREAD_LOOP_ERROR = 15001;
constexpr int ERR_THREAD_POOL_DESTROY_POOL_ERROR = 15002;

static std::unordered_map<int, std::string> error_messages = {
    // General
    { ERR_UNKNOWN, "Unknown error occurred." },

    // Socket
    { ERR_SOCKET_GET_ADDR_INFO_FAILED, "Socket: failed to find address information." },
    { ERR_SOCKET_BIND_SOCKET_FAILED, "Socket: failed to open and bind socket." },
    { ERR_SOCKET_LISTEN_FAILED, "Socket: failed to start listening on bound socket." },
    { ERR_SOCKET_CONNECT_GETADDRINFO_FAILED, "Socket: failed to lookup target host and port." },
    { ERR_SOCKET_CONNECT_FAILED, "Socket: failed to connect to host after lookup." },
    { ERR_SOCKET_MAKE_NONBLOCKING_FAILED, "Socket: failed to make socket non-blocking." },
    { ERR_SOCKET_ACCEPT_CONN_FAILED, "Socket: failed to accept incoming connection." },
    { ERR_SOCKET_GET_HOST_FAILED, "Socket: failed to get host information." },
    { ERR_SOCKET_BUFFER_FULL, "Socket: incoming data exceeded buffer size." },
    { ERR_SOCKET_RECV_FAILED, "Socket: failed to receive incoming data." },
    { ERR_SOCKET_INVALID_SEND_ATTEMPT, "Socket: attempted to send on a closed or listening socket." },
    { ERR_SOCKET_SEND_FAILED, "Socket: failed to send data." },

    // SecureSocket
    { ERR_SECURE_SOCKET_HANDSHAKE_INIT_FAILED, "SecureSocket: failed to initialize handshake." },
    { ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_PARSE_FAILED, "SecureSocket: failed to parse handshake initialization." },
    { ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_DERIVE_FAILED, "SecureSocket: failed to derive shared secret (peer)." },
    { ERR_SECURE_SOCKET_HANDSHAKE_ACCEPT_SEND_FAILED, "SecureSocket: failed to send handshake response." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_PARSE_FAILED, "SecureSocket: failed to parse handshake response." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_DERIVE_FAILED, "SecureSocket: failed to derive shared secret (host)." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_HASH_FAILED, "SecureSocket: failed to generate symmetric key (host)." },
    { ERR_SECURE_SOCKET_HANDSHAKE_FINAL_FAILED, "SecureSocket: failed to finalize handshake." },
    { ERR_SECURE_SOCKET_HANDSHAKE_CONFIRM_DERIVE_FAILED, "SecureSocket: failed to generate symmetric key (peer)." },
    { ERR_SECURE_SOCKET_HANDSHAKE_CONFIRM_SEND_FAILED, "SecureSocket: failed to send confirmation of handshake." },
    { ERR_SECURE_SOCKET_RECV_FAILED, "SecureSocket: failed to receive incoming data" },
    { ERR_SECURE_SOCKET_SEND_FAILED, "SecureSocket: failed to send data." },

    // Context
    { ERR_CONTEXT_BUFFER_FULL, "Context: incoming data exceeded context buffer size" },
    { ERR_CONTEXT_HANDLE_REQUEST_FAILED, "Context: failed to handle request" },
    { ERR_CONTEXT_HANDLE_READ_FAILED, "Context: failed to read incoming data" },
    { ERR_CONTEXT_DO_ERROR_FAILED, "Context: failed to send error response to peer" },
    { ERR_CONTEXT_PING_FAILED, "Context: failed to send ping response to peer" },
    { ERR_CONTEXT_SEND_MESSAGE_FAILED, "Context: failed to send message to peer" },

    // Server
    { ERR_SERVER_ACCEPT_CONN_FAILED, "Server: failed to accept incoming connection" },

    // ThreadPool
    { ERR_THREAD_POOL_THREAD_LOOP_ERROR, "ThreadPool: error occurred in task loop" },
    { ERR_THREAD_POOL_DESTROY_POOL_ERROR, "ThreadPool: error occurred destroying pool" },
};

#endif