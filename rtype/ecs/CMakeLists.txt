cmake_minimum_required(VERSION 3.16)
project(rtype_ecs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(rtype_ecs SHARED
    src/engine.cpp
)

target_include_directories(rtype_ecs
    PUBLIC include
)
