#!/bin/bash
# Quick build script - builds everything
# Author: /x64/dumped

set -e

echo "========================================"
echo "Snow-Pi Dashboard - Complete Build"
echo "========================================"
echo ""

# Check for dependencies
echo "Checking dependencies..."
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Installing dependencies..."
    make install-deps-debian
fi

# Step 1: Extract SDL3 if needed
if [ ! -d "SDL-release-3.2.26/SDL-release-3.2.26" ]; then
    if [ -f "SDL-release-3.2.26.zip" ]; then
        echo "Extracting SDL3..."
        unzip -q SDL-release-3.2.26.zip
    else
        echo "Error: SDL-release-3.2.26.zip not found"
        exit 1
    fi
fi

# Step 2: Build SDL3
if [ ! -f "SDL-release-3.2.26/build/libSDL3.so" ]; then
    echo "Building SDL3..."
    ./build-sdl3.sh
else
    echo "SDL3 already built ✓"
fi

# Step 3: Build SDL3_ttf
if [ ! -f "SDL_ttf/build/libSDL3_ttf.so" ]; then
    echo "Building SDL3_ttf..."
    ./build-sdl3-ttf.sh
else
    echo "SDL3_ttf already built ✓"
fi

# Step 4: Build dashboard
echo ""
echo "Building dashboard..."
make

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo ""
echo "Run with: ./snow-pi-dash"
echo ""

