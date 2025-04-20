#!/bin/bash

# === CONFIGURATION ===
HEX_FILE="firmware/build/firmware.ino.hex"  # Path to compiled Arduino firmware
PROGRAMMER="arduino"  # Programmer type (Arduino as ISP)
MCU="atmega328pb"

# Check if SERIAL_PORT was provided as an argument
if [[ -z "$1" ]]; then
    echo "Error: No serial port provided."
    echo "Usage: ./upload_firmware.sh <serial_port>"
    exit 1
fi

SERIAL_PORT="$1"

# Check if the HEX file exists
if [[ ! -f "$HEX_FILE" ]]; then
    echo "Error: Compiled HEX file not found. Run './build_firmware.sh' first."
    exit 1
fi

# Check if the serial port exists
if [[ ! -e "$SERIAL_PORT" ]]; then
    echo "Error: Serial port '$SERIAL_PORT' not found."
    echo "Available ports:"
    ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "No serial ports found."
    exit 1
fi

# Test if avrdude can communicate with the programmer
echo "Checking connection with programmer on '$SERIAL_PORT'..."
set -x 
avrdude -p "$MCU" -c "$PROGRAMMER" -P "$SERIAL_PORT" -b 19200 -n > /dev/null 2>&1
set +x 
if [[ $? -ne 0 ]]; then
    echo "Error: Programmer not responding on '$SERIAL_PORT'."
    echo "Ensure the device is connected and try again."
    exit 1
fi

# Flash the firmware
echo "Uploading firmware to '$SERIAL_PORT'..."
avrdude -p "$MCU" -c "$PROGRAMMER" -P "$SERIAL_PORT" -b 19200 -U flash:w:"$HEX_FILE":i

# Check if flashing was successful
if [[ $? -ne 0 ]]; then
    echo "Error: Upload failed!"
    exit 1
fi

echo "Firmware uploaded successfully."

