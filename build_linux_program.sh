#!/bin/bash

# === CONFIGURATION ===
MAKEFILE_DIR="$(dirname "$0")/linux"  # Reference makeFile in linux directory
BUILD_DIR="build"  # Output to build directory

echo "=================================="
echo " Building Linux Progam "
echo "=================================="

# Ensure the build directory exists
mkdir -p "$MAKEFILE_DIR/$BUILD_DIR"

# Run Make inside the linux directory
make -C "$MAKEFILE_DIR"

# Check if Make was successful
if [[ $? -ne 0 ]]; then
    echo "Program build failed!"
    exit 1
fi

echo "Linux program built successfully."
echo "=================================="
echo " Program output to '/build/'"
echo "=================================="

