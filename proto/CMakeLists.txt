# file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# add_library(ProtoInternal ${PROTO_HDRS} ${PROTO_SRCS})
# target_link_libraries(ProtoInternal protobuf::libprotobuf)

# set(ProtobufIncludePath ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "Protobuf include path.")

file(GLOB PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/*.proto")

add_library(ProtoInternal OBJECT ${PROTO_FILES})
target_link_libraries(ProtoInternal PUBLIC protobuf::libprotobuf)
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(ProtoInternal PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")

protobuf_generate(
    LANGUAGE cpp
    TARGET ProtoInternal
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
)