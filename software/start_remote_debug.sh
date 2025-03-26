#!/bin/bash

# Usage:
#   ./start_remote_gdbserver.sh <ip> <user> <password>
# Example:
#   ./start_remote_gdbserver.sh 192.168.1.79 pi raspberry

REMOTE_IP="$1"
REMOTE_USER="$2"
REMOTE_PASS="$3"
REMOTE_PORT=2345
REMOTE_EXE="/home/$REMOTE_USER/bin/linux_camera"

# Validate arguments
if [[ -z "$REMOTE_IP" || -z "$REMOTE_USER" || -z "$REMOTE_PASS" ]]; then
    echo "Usage: $0 <remote_ip> <remote_user> <remote_password>"
    exit 1
fi

echo "[INFO] Checking connectivity to $REMOTE_IP..."
ping -c 1 -W 2 "$REMOTE_IP" > /dev/null
if [[ $? -ne 0 ]]; then
    echo "[ERROR] Cannot reach $REMOTE_IP. Check network connection."
    exit 1
fi

echo "[INFO] Validating SSH access..."

sshpass -p "$REMOTE_PASS" ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no "$REMOTE_USER@$REMOTE_IP" "echo connected" >/dev/null
if [[ $? -ne 0 ]]; then
    echo "[ERROR] SSH authentication failed for $REMOTE_USER@$REMOTE_IP"
    exit 1
fi

echo "[INFO] Checking for gdbserver on remote..."
sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no "$REMOTE_USER@$REMOTE_IP" "which gdbserver" >/dev/null
if [[ $? -ne 0 ]]; then
    echo "[ERROR] gdbserver is not installed on the remote system."
    echo "Install it using: sudo apt install gdbserver"
    exit 1
fi

echo "[INFO] Checking for executable: $REMOTE_EXE..."
sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no "$REMOTE_USER@$REMOTE_IP" "test -x $REMOTE_EXE"
if [[ $? -ne 0 ]]; then
    echo "[ERROR] Executable not found or not executable: $REMOTE_EXE"
    exit 1
fi

echo "[INFO] Killing any existing gdbserver processes..."
sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no "$REMOTE_USER@$REMOTE_IP" "pkill -f 'gdbserver.*$REMOTE_EXE'" 2>/dev/null

echo "[INFO] Starting gdbserver on $REMOTE_USER@$REMOTE_IP:$REMOTE_PORT..."
sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no "$REMOTE_USER@$REMOTE_IP" \
    "nohup gdbserver :$REMOTE_PORT $REMOTE_EXE >/dev/null 2>&1 &"

if [[ $? -ne 0 ]]; then
    echo "[ERROR] Failed to start gdbserver on $REMOTE_IP"
    exit 1
fi

echo "[SUCCESS] gdbserver started on $REMOTE_IP:$REMOTE_PORT"
exit 0

