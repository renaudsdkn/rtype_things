cmake_minimum_required(VERSION 3.16)
project(R-Type LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# === CPM AVEC CACHE STABLE ===
set(CPM_DIR "${CMAKE_BINARY_DIR}/cpm_cache")
set(CPM_PATH "${CPM_DIR}/CPM_0.38.1.cmake")

# Créer le dossier cache si besoin
if(NOT EXISTS ${CPM_DIR})
    file(MAKE_DIRECTORY ${CPM_DIR})
endif()

# Télécharger UNE SEULE FOIS
if(NOT EXISTS ${CPM_PATH})
    message(STATUS "Downloading CPM.cmake...")
    file(DOWNLOAD 
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.1/CPM.cmake
        ${CPM_PATH}
        STATUS download_status
    )
    
    # Vérifier le téléchargement
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
    OPTIONS "ASIO_STANDALONE ON"
)

# Tes sous-répertoires
add_subdirectory(rtype/ecs)
add_subdirectory(rtype/protocol) 
add_subdirectory(rtype/server)
add_subdirectory(rtype/client)