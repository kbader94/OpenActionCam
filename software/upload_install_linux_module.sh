#!/bin/bash

set -euo pipefail

# === Arguments ===
IP="$1"
USER="$2"
PASS="$3"

# === Paths ===
LOCAL_MODULE_DIR="$(dirname "$0")/linux/oac"
REMOTE_DIR="/home/$USER/oac"

# === Helper ===
check() {
    local status=$1
    local msg=$2
    if [ $status -ne 0 ]; then
        echo "[✘] Error: $msg"
        exit $status
    fi
}

# === Check dependencies ===
if ! command -v sshpass &> /dev/null; then
    echo "[✘] Error: sshpass is required. Install it with: sudo apt install sshpass"
    exit 1
fi

if ! command -v rsync &> /dev/null; then
    echo "[✘] Error: rsync is required. Install it with: sudo apt install rsync"
    exit 1
fi

# === Sync code to remote ===
echo "[*] Syncing local module directory to RPi..."
sshpass -p "$PASS" rsync -az --delete -e ssh "$LOCAL_MODULE_DIR/" "$USER@$IP:$REMOTE_DIR/"
check $? "Failed to sync module directory to RPi"

# === Install and detect reboot ===
echo "[*] Running 'make install' remotely..."
REBOOT_FLAG=$(sshpass -p "$PASS" ssh -o ConnectTimeout=3 "$USER@$IP" \
  "cd $REMOTE_DIR && make install && echo '__NO_REBOOT__'" || echo "__SSH_FAIL__")

if [[ "$REBOOT_FLAG" == "__SSH_FAIL__" ]]; then
  echo "[✘] SSH or 'make install' failed — check logs."
  exit 1
fi

# === If no reboot occurred, reload modules live ===
if echo "$REBOOT_FLAG" | grep -q "__NO_REBOOT__"; then
    echo "[*] Detected: system did NOT reboot — reloading kernel modules live"

    sshpass -p "$PASS" ssh "$USER@$IP" bash -c "'
        echo \"[*] Unloading existing modules (ignore errors if not loaded)...\"
        sudo rmmod oac_watchdog_driver 2>/dev/null || true
        sudo rmmod oac_driver 2>/dev/null || true

        echo \"[*] Reloading updated modules...\"
        sudo modprobe oac_driver && sudo modprobe oac_watchdog_driver
    '"
    check $? "Failed to reload modules live"

else
    echo "[*] Detected: system is rebooting (likely due to DT overlay change)"
    echo "[*] Waiting for RPi to come back online..."

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
