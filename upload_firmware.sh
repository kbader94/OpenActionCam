#!/bin/bash

# === CONFIGURATION ===
HEX_FILE="build/arduino/main.ino.hex" # Arduino firmware
PORT="/dev/ttyUSB0"  # Change this to your Arduino ISP port
PROGRAMMER="arduino"  # Programmer type (Arduino as ISP)
MCU="atmega328p"

if [[ ! -f "$HEX_FILE" ]]; then
    echo "❌ Error: Compiled HEX file not found! Run ./build.sh first."
    exit 1
fi

echo "=================================="
echo " Uploading Arduino Firmware"
echo "=================================="
avrdude -p "$MCU" -c "$PROGRAMMER" -P "$PORT" -b 19200 -U flash:w:"$HEX_FILE":i

if [[ $? -ne 0 ]]; then
    echo "❌ Upload failed!"
    exit 1
fi

echo "Upload successful!"
