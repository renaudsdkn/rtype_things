cmake_minimum_required(VERSION 3.16)
project(R-Type LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# === CPM AVEC CACHE DANS BUILD/ ===
set(CPM_DIR "${CMAKE_BINARY_DIR}/cpm_modules")
set(CPM_PATH "${CPM_DIR}/CPM_0.38.1.cmake")

# Cache des sources dans build/ (ignoré par Git)
set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/_deps")

# Créer le dossier pour CPM.cmake
if(NOT EXISTS ${CPM_DIR})
    file(MAKE_DIRECTORY ${CPM_DIR})
endif()

# Télécharger CPM UNE SEULE FOIS
if(NOT EXISTS ${CPM_PATH})
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD 
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.1/CPM.cmake
        ${CPM_PATH}
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download CPM.cmake")
    endif()
endif()

include(${CPM_PATH})
# === FIN CPM ===

# === ASIO ===
CPMAddPackage(
    NAME asio
    GITHUB_REPOSITORY chriskohlhoff/asio
    GIT_TAG asio-1-28-0
    DOWNLOAD_ONLY YES
)

if(asio_ADDED)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
endif()

# === RAYLIB (via archive ZIP - RAPIDE) ===
CPMAddPackage(
    NAME raylib
    VERSION 5.0
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    OPTIONS
        "BUILD_EXAMPLES OFF"
        "BUILD_GAMES OFF"
)

# === NLOHMANN JSON ===
CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.11.3
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
    OPTIONS
        "JSON_BuildTests OFF"
)

# === CPP-HTTPLIB ===
CPMAddPackage(
    NAME httplib
    GITHUB_REPOSITORY yhirose/cpp-httplib
    GIT_TAG v0.14.3
    OPTIONS
        "HTTPLIB_REQUIRE_OPENSSL OFF"
        "HTTPLIB_REQUIRE_ZLIB OFF"
)

# Sous-répertoires
add_subdirectory(rtype/ecs)
add_subdirectory(rtype/protocol) 
add_subdirectory(rtype/server)
add_subdirectory(rtype/client)