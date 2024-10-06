# The order of this source-list isn't essential but does determine the order of execution of tests.
# As such, they are currently ordered hierarchically; later tests and fixtures rely on the correctness of earlier tests.
# E.g assumptions can be made about socket & server functionality in context.cpp because of the tests in socket.cpp, server.cpp, etc.

target_sources(server_test
    PRIVATE
        main.cpp
        buffer.cpp
        circular-buffer.cpp
        socket.cpp
        secure-socket.cpp
        context.cpp
        server.cpp
)
