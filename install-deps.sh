#!/bin/bash
# Install all dependencies for Snow-Pi Dashboard
# Author: /x64/dumped

echo "=========================================="
echo "Installing Snow-Pi Dashboard Dependencies"
echo "=========================================="
echo ""

# Detect OS
if [ -f /etc/debian_version ]; then
    echo "Detected Debian/Ubuntu system"
    echo ""
    
    sudo apt-get update
    
    echo "Installing build tools..."
    sudo apt-get install -y build-essential cmake git pkg-config
    
    echo "Installing SDL3 dependencies..."
    sudo apt-get install -y \
        libx11-dev libxext-dev libwayland-dev libxkbcommon-dev \
        libegl1-mesa-dev libgles2-mesa-dev libdbus-1-dev \
        libibus-1.0-dev libudev-dev
    
    echo "Installing SDL3_ttf dependencies..."
    sudo apt-get install -y libfreetype6-dev libharfbuzz-dev
    
    echo "Installing fonts..."
    sudo apt-get install -y fonts-dejavu-core
    
elif [ -f /etc/arch-release ]; then
    echo "Detected Arch Linux system"
    echo ""
    
    sudo pacman -Syu --needed base-devel cmake git pkg-config \
        wayland libxkbcommon mesa libx11 libxext dbus ibus \
        freetype2 harfbuzz ttf-dejavu
        
else
    echo "Unsupported OS. Please install dependencies manually."
    echo "Required: cmake, git, pkg-config, freetype2, harfbuzz"
    exit 1
fi

echo ""
echo "=========================================="
echo "Dependencies installed successfully!"
echo "=========================================="
echo ""
echo "Now run: ./quick-build.sh"
echo ""

