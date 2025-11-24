#!/bin/bash
# SDL3 Build Script for Snow-Pi Dashboard
# Author: /x64/dumped

set -e

echo "=========================================="
echo "Building SDL3 for Snow-Pi Dashboard"
echo "=========================================="

SDL_SOURCE="SDL-release-3.2.26/SDL-release-3.2.26"
BUILD_DIR="SDL-release-3.2.26/build"

# Check if SDL3 source exists
if [ ! -d "$SDL_SOURCE" ]; then
    echo "Error: SDL3 source not found at $SDL_SOURCE"
    echo "Make sure you've extracted SDL-release-3.2.26.zip"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring SDL3..."
cmake ../SDL-release-3.2.26 \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_STATIC=OFF \
    -DSDL_SHARED=ON \
    -DSDL_TEST_LIBRARY=OFF

# Build
echo "Building SDL3 (this may take a few minutes)..."
cmake --build . --parallel

echo ""
echo "=========================================="
echo "SDL3 build complete!"
echo "=========================================="
echo ""
echo "Now you can build the dashboard with:"
echo "  make"
echo ""

