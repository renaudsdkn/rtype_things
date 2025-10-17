#!/bin/bash
echo "🧹 Cleaning EVERYTHING..."

# Remove build directory completely
if [ -d "build" ]; then
    echo "🗑️  Removing build directory..."
    rm -rf build/
fi

# Remove Conan generated files
echo "🧽 Removing generated files..."
rm -f asio-*.cmake cmakedeps_macros.cmake CMakePresets.json
rm -f conan*.sh conan*.cmake deactivate_*.sh

echo "✅ Project completely cleaned!"