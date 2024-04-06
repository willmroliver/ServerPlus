cmake_minimum_required(VERSION 3.28)

project(ServerPlus VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ABSL_LLVM_FLAGS}")
set(Boost_LIBRARIES ${Boost_INCLUDE_DIRS}/stage/lib)
set(Boost_USE_STATIC_LIBS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(ENV{LLVM_PROFILE_FILE} "default.profraw")
endif()

find_package(Boost 1.82.0 COMPONENTS unit_test_framework REQUIRED)

find_package(Protobuf CONFIG REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(Libevent REQUIRED)
find_package(LibeventPlus REQUIRED)

find_package(OpenSSL REQUIRED)
find_package(CryptPlus REQUIRED)

add_subdirectory(proto)
include_directories(${ProtobufIncludePath})

# Setup library
add_library(ServerPlus "")
add_subdirectory(src)

target_link_libraries(ServerPlus
    PUBLIC
        LibeventPlus
        CryptPlus
        ProtoInternal
)

target_include_directories(ServerPlus
    PUBLIC
        include
)

# Setup test executable
add_executable(server_test)
add_subdirectory(test)

target_link_libraries(server_test
    PRIVATE
        ServerPlus
        ${Boost_LIBRARIES}
        LibeventPlus
)

target_include_directories(server_test 
    PRIVATE 
        include
        test/include
        ${Boost_INCLUDE_DIRS}
)