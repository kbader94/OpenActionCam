#!/bin/bash

set -euo pipefail

# === Arguments ===
IP="$1"
USER="$2"
PASS="$3"

# === Paths ===
LOCAL_OACD_DIR="$(realpath "$(dirname "$0")/linux/oacd")"
REMOTE_DIR="/home/$USER/oacd"

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
echo "[*] Syncing oacd project to $USER@$IP:$REMOTE_DIR ..."
sshpass -p "$PASS" rsync -az --delete -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
  "$LOCAL_OACD_DIR/" "$USER@$IP:$REMOTE_DIR/"


# === Run make install remotely ===
echo "[*] Running make install remotely..."
sshpass -p "$PASS" ssh -o ConnectTimeout=3 "$USER@$IP" "cd $REMOTE_DIR && make install"

echo "[✔] oacd upload and install complete."
