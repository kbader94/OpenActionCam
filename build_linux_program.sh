#!/bin/bash

# === CONFIGURATION ===
MAKEFILE_DIR="$(dirname "$0")/linux"  # Reference Makefile in linux directory
BUILD_DIR="build"  # Output to build directory
CROSS_COMPILER="aarch64-linux-gnu-gcc"

echo "=================================="
echo " Checking Dependencies "
echo "=================================="

# Check if the cross-compiler is installed
if ! command -v "$CROSS_COMPILER" &> /dev/null; then
    echo "Error: Cross-compiler '$CROSS_COMPILER' is not installed."

    # Attempt to install on Debian-based systems
    if [[ -x "$(command -v apt)" ]]; then
        echo "Installing cross-compiler using apt..."
        sudo apt update && sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
    elif [[ -x "$(command -v pacman)" ]]; then
        echo "Installing cross-compiler using pacman..."
        sudo pacman -S aarch64-linux-gnu-gcc
    elif [[ -x "$(command -v brew)" ]]; then
        echo "Installing cross-compiler using Homebrew..."
        brew install aarch64-none-linux-gnu
    else
        echo "Error: Unsupported package manager. Install '$CROSS_COMPILER' manually."
        exit 1
    fi

    # Verify installation
    if ! command -v "$CROSS_COMPILER" &> /dev/null; then
        echo "Error: Cross-compiler installation failed. Please install it manually."
        exit 1
    fi
fi

echo "Cross-compiler '$CROSS_COMPILER' found."

echo "=================================="
echo " Building Linux Program "
echo "=================================="

# Ensure the build directory exists
mkdir -p "$MAKEFILE_DIR/$BUILD_DIR"

# Run Make inside the linux directory
make -C "$MAKEFILE_DIR" ARCH=arm64

# Check if Make was successful
if [[ $? -ne 0 ]]; then
    echo "Error: Program build failed!"
    exit 1
fi

echo "Linux program built successfully."
echo "=================================="
echo " Program output to '/build/'"
echo "=================================="

