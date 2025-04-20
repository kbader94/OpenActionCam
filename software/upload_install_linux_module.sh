#!/bin/bash

set -euo pipefail

# Arguments
IP="$1"
USER="$2"
PASS="$3"

# Local path (relative to script location)
LOCAL_MODULE_DIR="$(dirname "$0")/linux/oac"
REMOTE_DIR="/home/$USER/oac"

# Function to check exit code and report error
check() {
    local status=$1
    local msg=$2
    if [ $status -ne 0 ]; then
        echo "[✘] Error: $msg"
        exit $status
    fi
}

# Check dependencies
if ! command -v sshpass &> /dev/null; then
    echo "[✘] Error: sshpass is required. Install it with: sudo apt install sshpass"
    exit 1
fi

echo "[*] Copying local module directory to RPi..."
sshpass -p "$PASS" scp -r "$LOCAL_MODULE_DIR" "$USER@$IP:$REMOTE_DIR"
check $? "Failed to copy module directory to RPi"

# Try running make install remotely and capture whether the system reboots
echo "[*] Running 'make install' remotely..."
REBOOT_FLAG=$(sshpass -p "$PASS" ssh -o ConnectTimeout=3 "$USER@$IP" \
  "cd $REMOTE_DIR && make install && echo '__NO_REBOOT__'" || true)

# If the system rebooted, the SSH connection will drop and the flag won't be returned
if echo "$REBOOT_FLAG" | grep -q "__NO_REBOOT__"; then
    echo "[*] Detected: system did NOT reboot — loading kernel modules live"
    sshpass -p "$PASS" ssh "$USER@$IP" "sudo modprobe oac_driver && sudo modprobe oac_watchdog_driver"
    check $? "Failed to modprobe modules live"
else
    echo "[*] Detected: system is rebooting (likely due to DT overlay change)"
    echo "[*] Waiting for RPi to come back online..."

    # Wait for ping to succeed
    for i in {1..30}; do
        if ping -c 1 -W 1 "$IP" &> /dev/null; then
            echo "[*] Device is responding to ping."
            break
        fi
        sleep 2
    done

    echo "[*] Waiting for SSH to become available..."
    for i in {1..30}; do
        if sshpass -p "$PASS" ssh -o ConnectTimeout=2 "$USER@$IP" "echo '[*] SSH is up.'" &> /dev/null; then
            echo "[*] SSH is back online. RPi reboot complete."
            break
        fi
        sleep 2
    done
fi

echo "[✔] Upload and install process completed."
