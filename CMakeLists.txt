cmake_minimum_required(VERSION 3.26)

project(DB VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -pedantic -Wall -Werror")

set(Boost_LIBRARIES ${Boost_INCLUDE_DIRS}/stage/lib)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.82.0 COMPONENTS unit_test_framework REQUIRED)
find_package(Libevent REQUIRED)
find_package(LibeventPlus REQUIRED)

add_library(db_lib "")

add_executable(db_test)
add_executable(db_exec src/main.cpp)

add_subdirectory(test)
add_subdirectory(src)

target_include_directories(db_lib
    PRIVATE
        include
)

target_include_directories(db_exec 
    PRIVATE 
        include
)

target_include_directories(db_test 
    PRIVATE 
        include
        test/include
    PUBLIC 
        ${Boost_INCLUDE_DIRS}
)