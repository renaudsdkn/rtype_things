cmake_minimum_required(VERSION 3.16)
project(rtype_protocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(rtype_protocol SHARED
    src/protocol.cpp
    src/serializer.cpp
    src/factory.cpp
    src/protocol_part2.cpp
)

target_include_directories(rtype_protocol
    PUBLIC include
)
