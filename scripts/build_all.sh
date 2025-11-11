#!/bin/bash
cd "$(dirname "$0")/.."

PROJECT_ROOT="$(pwd)"
echo "📂 Project root: $PROJECT_ROOT"

if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: CMakeLists.txt not found!"
    exit 1
fi

# TOUJOURS TOUT BUILDER
BUILD_TYPE="Debug"

mkdir -p build
cd build

echo "⚙️ Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

echo "🔨 Building ALL targets..."
# Builder TOUT, pas juste le serveur
cmake --build . --parallel

echo "✅ All builds completed!"

# Affiche les binaires générés
echo ""
echo "📦 Generated binaries:"
find . -name "r-type_*" -type f -executable 2>/dev/null || echo "No binaries found"