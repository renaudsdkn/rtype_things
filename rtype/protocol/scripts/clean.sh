#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

echo "🧹 Cleaning Protocol library..."
cd "$PROJECT_ROOT/build"

# Clean la cible Protocol
cmake --build . --target clean -- -j 4

# Supprime la librairie et les fichiers objets
rm -f rtype/protocol/librtype_protocol.so
rm -f rtype/protocol/CMakeFiles/rtype_protocol.dir/src/*.o
rm -f rtype/protocol/CMakeFiles/rtype_protocol.dir/src/*.o.d

echo "✅ Protocol cleaned!"