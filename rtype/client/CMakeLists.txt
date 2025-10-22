cmake_minimum_required(VERSION 3.16)
project(rtype_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0  # ou la version demandée par ton sujet
)
FetchContent_MakeAvailable(raylib)

find_package(raylib REQUIRED)

add_executable(r-type_client
    src/main.cpp
    src/client.cpp
    src/GameClient.cpp
    src/InputManager.cpp
    src/Renderer.cpp
    src/SpriteManager.cpp
    src/Starfield.cpp
    src/Game.cpp
)

target_include_directories(r-type_client PRIVATE 
    ${CMAKE_BINARY_DIR}/_deps/asio-src/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/rtype/protocol/include
    ${CMAKE_SOURCE_DIR}/rtype/ecs/include
)

target_link_libraries(r-type_client PRIVATE 
    rtype_ecs
    rtype_protocol
    raylib
    pthread
)