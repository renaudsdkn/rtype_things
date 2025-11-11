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
    src/chatServer.cpp
    src/ChatManager.cpp
    src/ScoreFileWriter.cpp
    src/AdminAPI.cpp
)

# INCLUDES - Utiliser la variable asio_SOURCE_DIR définie par CPM
target_include_directories(r-type_server PRIVATE 
    ${asio_SOURCE_DIR}/asio/include          # ← Changement ici
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/rtype/protocol/include
    ${CMAKE_SOURCE_DIR}/rtype/ecs/include
)

# LIENS (asio est header-only, on le link quand même pour les defs)
target_link_libraries(r-type_server PRIVATE 
    asio              # ← Ajouter cette ligne
    rtype_ecs
    rtype_protocol
    nlohmann_json::nlohmann_json
    httplib::httplib  #
    pthread
)