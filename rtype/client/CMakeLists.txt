cmake_minimum_required(VERSION 3.16)
project(rtype_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(r-type_client
    src/main.cpp
    src/client.cpp
    src/GameClient.cpp
    src/InputManager.cpp
    src/Renderer.cpp
    src/SpriteManager.cpp
    src/Starfield.cpp
    src/Game.cpp
    src/Datagame.cpp
    src/Anima.cpp
    src/chat.cpp
    src/chatClient.cpp
    src/chatUI.cpp
)

target_include_directories(r-type_client PRIVATE 
    ${asio_SOURCE_DIR}/asio/include          # ← AJOUTER cette ligne
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/rtype/protocol/include
    ${CMAKE_SOURCE_DIR}/rtype/ecs/include
    ${CMAKE_SOURCE_DIR}/rtype/server/include
)

target_link_libraries(r-type_client PRIVATE 
    asio              # ← AJOUTER cette ligne
    rtype_ecs
    rtype_protocol
    raylib
    nlohmann_json::nlohmann_json
    pthread
)