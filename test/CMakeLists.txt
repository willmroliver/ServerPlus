target_sources(server_test
    PRIVATE
        main.cpp
        server.cpp
        buffer.cpp
)

target_link_libraries(server_test
    PRIVATE
        ServerPlus
    PUBLIC 
        ${Boost_LIBRARIES}
        LibeventPlus
)