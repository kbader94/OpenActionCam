#!/bin/bash

# === CONFIGURATION ===
BOARD="arduino:avr:uno"  # Change to match your board
SKETCH="arduino/main.ino" # Path to Arduino sketch
LINUX_SRC="linux/main.c"  # Linux program entry point
OUTPUT_DIR="build"
ARDUINO_BUILD_DIR="$OUTPUT_DIR/arduino"
LINUX_BUILD_DIR="$OUTPUT_DIR/linux"

# Ensure build directories exist
mkdir -p "$ARDUINO_BUILD_DIR" "$LINUX_BUILD_DIR"

echo "=================================="
echo " Building Arduino Firmware"
echo "=================================="
arduino-cli compile --fqbn "$BOARD" --output-dir "$ARDUINO_BUILD_DIR" "$SKETCH"
if [[ $? -ne 0 ]]; then
    echo "Arduino build failed!"
    exit 1
fi
echo "✅ Arduino firmware built successfully."

echo "=================================="
echo " Building Linux Program"
echo "=================================="
gcc -Wall -o "$LINUX_BUILD_DIR/program" "$LINUX_SRC" comms.c -I. -lpthread
if [[ $? -ne 0 ]]; then
    echo "Linux build failed!"
    exit 1
fi
echo "✅ Linux program built successfully."

echo "=================================="
echo " Build Complete!"
echo " Output stored in '$OUTPUT_DIR'"
echo "=================================="
