#!/bin/bash

# Parameters
RPI_HOST="$1"         # IP or hostname
RPI_USER="$2"         # SSH username
RPI_PASS="$3"         # SSH password
RESET_GPIO="${4:-18}"               # GPIO pin (default 18)
SERIAL_PORT="${5:-/dev/serial0}"    # Serial port (default /dev/serial0)
BAUD="${6:-115200}" 

# Resolve script directory and relative paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOCAL_HEX="$SCRIPT_DIR/firmware/build/firmware.ino.hex"
LOCAL_SCRIPT="$SCRIPT_DIR/flash328_from_mcp.sh"

REMOTE_HEX="/tmp/firmware.hex"
REMOTE_SCRIPT="/usr/local/bin/flash328_from_mcp.sh"



if [[ -z "$RPI_HOST" || -z "$RPI_USER" || -z "$RPI_PASS" ]]; then
    echo "Usage: $0 <rpi_host> <rpi_user> <rpi_pass> [gpio] [serial] [baud]"
    echo "Uploads the firmware to the rpi at the specified IP, then flashes it to the atmega328"
    echo "  -rpi_host: IP or hostname of RPi"
    echo "  -rpi_user: SSH username for RPi"
    echo "  -rpi_pass: SSH password for RPi"
    echo "  -gpio: GPIO pin to reset the RPi (default 18)"
    echo "  -serial: Serial port (default /dev/serial0)"
    echo "  -baud: Baud rate (default 9600)"
    exit 1
fi

if [[ ! -f "$LOCAL_HEX" ]]; then
    echo "[!] Firmware hex not found at $LOCAL_HEX"
    exit 2
fi

if [[ ! -f "$LOCAL_SCRIPT" ]]; then
    echo "[!] Local flash script not found at $LOCAL_SCRIPT"
    exit 3
fi

echo "[*] Checking for remote flash script..."
sshpass -p "$RPI_PASS" ssh "$RPI_USER@$RPI_HOST" "test -f $REMOTE_SCRIPT || sudo tee $REMOTE_SCRIPT > /dev/null" < "$LOCAL_SCRIPT"
sshpass -p "$RPI_PASS" ssh "$RPI_USER@$RPI_HOST" "sudo chmod +x $REMOTE_SCRIPT"

echo "[*] Copying firmware to RPi..."
sshpass -p "$RPI_PASS" scp "$LOCAL_HEX" "$RPI_USER@$RPI_HOST:$REMOTE_HEX"

echo "[*] Flashing firmware remotely..."
sshpass -p "$RPI_PASS" ssh "$RPI_USER@$RPI_HOST" "sudo $REMOTE_SCRIPT -f $REMOTE_HEX -g $RESET_GPIO -p $SERIAL_PORT -b $BAUD"
