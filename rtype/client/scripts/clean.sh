#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

echo "🧹 Cleaning..."
cd "$PROJECT_ROOT/build"

# Clean CMake
cmake --build . --target clean

# Supprime les fichiers objets spécifiques au client
rm -f rtype/client/CMakeFiles/r-type_client.dir/src/*.o
rm -f rtype/client/CMakeFiles/r-type_client.dir/src/*.o.d

echo "✅ Client cleaned!"