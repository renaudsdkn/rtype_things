cmake_minimum_required(VERSION 3.16)
project(rtype_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# EXÉCUTABLE
add_executable(r-type_server
    src/main.cpp
    src/server.cpp
    src/network.cpp
    src/game_loop.cpp
    src/message_handler.cpp
    src/PlayerManager.cpp
    src/room.cpp
    src/GameManager.cpp

)

# INCLUDES SEULEMENT - ASIO est header-only
target_include_directories(r-type_server PRIVATE 
    ${CMAKE_BINARY_DIR}/_deps/asio-src/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/rtype/protocol/include
    ${CMAKE_SOURCE_DIR}/rtype/ecs/include
)

# LIENS (SUPPRIME 'asio' de la liste)
target_link_libraries(r-type_server PRIVATE 
    rtype_ecs
    rtype_protocol
    pthread
)