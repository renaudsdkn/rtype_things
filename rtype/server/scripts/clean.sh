#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

echo "🧹 Cleaning..."
cd "$PROJECT_ROOT/build"

# Clean CMake
cmake --build . --target clean

# Supprime les fichiers objets spécifiques au serveur
rm -f rtype/server/CMakeFiles/r-type_server.dir/src/*.o
rm -f rtype/server/CMakeFiles/r-type_server.dir/src/*.o.d

echo "✅ Server cleaned!"