#!/bin/bash
# SDL3_ttf Build Script for Snow-Pi Dashboard
# Author: /x64/dumped

set -e

echo "=========================================="
echo "Building SDL3_ttf for Snow-Pi Dashboard"
echo "=========================================="

SDL_TTF_REPO="https://github.com/libsdl-org/SDL_ttf.git"
SDL_TTF_DIR="SDL_ttf"
BUILD_DIR="SDL_ttf/build"

# Check for required dependencies
echo "Checking for FreeType..."
if ! pkg-config --exists freetype2; then
    echo ""
    echo "FreeType not found. Installing dependencies..."
    echo "This requires sudo access."
    echo ""
    sudo apt-get update
    sudo apt-get install -y libfreetype6-dev libharfbuzz-dev pkg-config
fi

# Check if SDL3 is built
if [ ! -f "SDL-release-3.2.26/build/libSDL3.so" ]; then
    echo "Error: SDL3 not found. Please build SDL3 first with ./build-sdl3.sh"
    exit 1
fi

# Clone SDL_ttf if not present
if [ ! -d "$SDL_TTF_DIR" ]; then
    echo "Cloning SDL_ttf..."
    git clone --branch main --depth 1 $SDL_TTF_REPO
fi

cd "$SDL_TTF_DIR"

# Create build directory
mkdir -p build
cd build

# Get absolute path to SDL3
SDL3_DIR=$(realpath ../../SDL-release-3.2.26/build)

echo "Configuring SDL3_ttf..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL3_DIR="$SDL3_DIR" \
    -DCMAKE_PREFIX_PATH="$SDL3_DIR" \
    -DBUILD_SHARED_LIBS=ON

echo "Building SDL3_ttf..."
cmake --build . --parallel

# Copy library to SDL3 build directory for easy linking
cp libSDL3_ttf*.so* "$SDL3_DIR/" 2>/dev/null || true

echo ""
echo "=========================================="
echo "SDL3_ttf build complete!"
echo "=========================================="
echo ""
echo "Now you can build the dashboard with:"
echo "  make"
echo ""

