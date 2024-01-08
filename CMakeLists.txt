cmake_minimum_required(VERSION 3.28)

project(ServerPlus VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ABSL_LLVM_FLAGS}")

set(Boost_LIBRARIES ${Boost_INCLUDE_DIRS}/stage/lib)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.82.0 COMPONENTS unit_test_framework REQUIRED)
find_package(absl REQUIRED)

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
add_subdirectory(proto)

find_package(Libevent REQUIRED)
find_package(LibeventPlus REQUIRED)

find_package(OpenSSL REQUIRED)
find_package(CryptPlus REQUIRED)

add_library(ServerPlus "")

add_executable(server_test)
add_subdirectory(src)
add_subdirectory(test)

target_include_directories(ServerPlus
    PRIVATE
        include
)

target_include_directories(server_test 
    PRIVATE 
        include
        test/include
    PUBLIC 
        ${Boost_INCLUDE_DIRS}
)