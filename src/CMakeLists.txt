target_sources(ServerPlus
    PRIVATE
        server.cpp
        context.cpp
)

target_link_libraries(ServerPlus
    PRIVATE
        ProtoInternal
    PUBLIC 
        LibeventPlus
        CryptPlus
        ${Protobuf_LIBRARIES}
)