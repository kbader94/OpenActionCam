#!/bin/bash

RESET_GPIO=17
SERIAL_PORT="/dev/serial0"
BAUD_RATE=115200

usage() {
    echo "Usage: $0 -f <firmware.hex> [-g <gpio>] [-p <serial_port>] [-b <baud_rate>]"
    exit 1
}

while getopts ":f:g:p:b:" opt; do
  case $opt in
    f) HEX_FILE="$OPTARG" ;;
    g) RESET_GPIO="$OPTARG" ;;
    p) SERIAL_PORT="$OPTARG" ;;
    b) BAUD_RATE="$OPTARG" ;;
    *) usage ;;
  esac
done

if [ -z "$HEX_FILE" ]; then
    echo "[!] Firmware hex file required."
    usage
fi

if [ ! -f "$HEX_FILE" ]; then
    echo "[!] Firmware file '$HEX_FILE' not found."
    exit 2
fi

if ! command -v avrdude > /dev/null; then
    echo "[!] Error: avrdude not found. Please install it with 'sudo apt install avrdude'"
    exit 3
fi

if ! command -v gpioset > /dev/null; then
    echo "[!] Error: gpioset not found. Please install it with 'sudo apt install gpiod'"
    exit 4
fi

echo "[*] Using GPIO$RESET_GPIO for reset (via gpiod)"
echo "[*] Using serial port: $SERIAL_PORT"
echo "[*] Baud rate: $BAUD_RATE"
echo "[*] Flashing: $HEX_FILE"

GPIOFIND=$(gpiofind "GPIO$RESET_GPIO")
if [ $? -ne 0 ]; then
    echo "[!] Failed to locate GPIO$RESET_GPIO via gpiofind. Try using a raw chip/line."
    exit 5
fi

CHIP=$(echo "$GPIOFIND" | awk '{print $1}')
LINE=$(echo "$GPIOFIND" | awk '{print $2}')

# Pulse reset low via gpiod (through 0.1uF cap)
gpioset "$CHIP" "$LINE"=0
sleep 0.1
gpioset "$CHIP" "$LINE"=1
sleep 0.2

# Flash firmware
avrdude -v -patmega328p -carduino -P "$SERIAL_PORT" -b "$BAUD_RATE" -D -U flash:w:"$HEX_FILE":i
