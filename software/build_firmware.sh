#!/bin/bash

# === CONFIGURATION ===
BOARD="arduino:avr:uno"  # Change to match your board
PLATFORM="arduino:avr"   # Required platform
SKETCH="firmware"        # Path to Arduino sketch FOLDER (not .ino file)
OUTPUT_DIR="build"
ARDUINO_BUILD_DIR="firmware/$OUTPUT_DIR"
REQUIRED_LIBS=("Adafruit NeoPixel") # List of required libraries

echo "=================================="
echo " Verifying Build Environment"
echo "=================================="

# Check if arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "Error: 'arduino-cli' is not installed."
    echo "Install it using: https://arduino.github.io/arduino-cli/installation/"
    exit 1
fi

# Check if the required platform is installed
if ! arduino-cli core list | grep -q "$PLATFORM"; then
    echo "Platform '$PLATFORM' is not installed. Installing now..."
    arduino-cli core install "$PLATFORM"
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to install '$PLATFORM'."
        exit 1
    fi
    echo "Platform '$PLATFORM' installed successfully."
fi

# Check if required libraries are installed
echo "Checking required libraries..."
for LIB in "${REQUIRED_LIBS[@]}"; do
    if ! arduino-cli lib list | grep -q "$LIB"; then
        echo "Library '$LIB' is missing. Installing now..."
        arduino-cli lib install "$LIB"
        if [[ $? -ne 0 ]]; then
            echo "Error: Failed to install library '$LIB'."
            exit 1
        fi
    fi
done
echo "All required libraries are installed."

# Check if the sketch folder exists
if [[ ! -d "$SKETCH" ]]; then
    echo "Error: Arduino sketch folder '$SKETCH' not found."
    echo "Ensure the sketch is inside a folder named 'firmware/' with 'firmware.ino'."
    exit 1
fi

# Ensure the build directory exists
if ! mkdir -p "$ARDUINO_BUILD_DIR"; then
    echo "Error: Failed to create build directory '$ARDUINO_BUILD_DIR'."
    exit 1
fi

echo "=================================="
echo " Building Firmware"
echo "=================================="

# Compile the firmware
arduino-cli compile --clean --fqbn "$BOARD" --output-dir "$ARDUINO_BUILD_DIR" "$SKETCH"
if [[ $? -ne 0 ]]; then
    echo "Firmware build failed."
    echo "Check the above error messages for more details."
    exit 1
fi

echo "Firmware built successfully."
echo "=================================="
echo " Output stored in '$ARDUINO_BUILD_DIR'"
echo "=================================="

