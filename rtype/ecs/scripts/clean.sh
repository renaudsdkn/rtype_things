#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

echo "🧹 Cleaning ECS library..."
cd "$PROJECT_ROOT/build"

# Clean la cible ECS
cmake --build . --target clean -- -j 4

# Supprime la librairie et les fichiers objets
rm -f rtype/ecs/librtype_ecs.a
rm -f rtype/ecs/CMakeFiles/rtype_ecs.dir/src/*.o
rm -f rtype/ecs/CMakeFiles/rtype_ecs.dir/src/*.o.d

echo "✅ ECS cleaned!"