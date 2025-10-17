#!/bin/bash
# Trouve la racine du projet automatiquement
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

echo "📂 Project root: $PROJECT_ROOT"
cd "$PROJECT_ROOT"

if [ ! -d "build" ]; then
    echo "📁 Creating build directory..."
    mkdir build
fi

cd build

if [ ! -f "CMakeCache.txt" ]; then
    echo "⚙️ Configuring CMake..."
    cmake ..
fi

cmake --build . --target r-type_server --parallel
echo "✅ Build completed!"