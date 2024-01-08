protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    host-handshake.proto
    peer-handshake.proto
)
add_library(ProtoInternal ${PROTO_HDRS} ${PROTO_SRCS})
target_link_libraries(ProtoInternal ${PROTOBUF_LIBRARIES})