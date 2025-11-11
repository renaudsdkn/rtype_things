#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../" && pwd)"

cd "$PROJECT_ROOT"

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

if [ ! -f "CMakeCache.txt" ]; then
    cmake ..
fi

# CORRECTION : c'est "rtype_protocol" sans tiret
cmake --build . --target rtype_protocol --parallel

echo "✅ Protocol library built: build/rtype/protocol/librtype_protocol.so"