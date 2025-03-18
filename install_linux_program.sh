#!/bin/bash

# === CONFIGURATION ===
BUILD_DIR="linux/build"
PROGRAM_NAME="linux_camera"
REMOTE_PATH="/home/$2/bin"  # Uses provided username for home directory

# === Check if sshpass is installed, install if missing ===
if ! command -v sshpass &> /dev/null; then
    echo "sshpass not found. Installing..."
    sudo apt update && sudo apt install -y sshpass
fi

# === Validate Input Arguments ===
if [[ -z "$1" || -z "$2" || -z "$3" ]]; then
    echo "Error: Remote IP, username, and password are required."
    echo "Usage: ./install_linux_program.sh <remote_IP> <username> <password>"
    exit 1
fi

REMOTE_IP="$1"
REMOTE_USER="$2"
REMOTE_PASS="$3"

# === Check if the compiled program exists ===
if [[ ! -f "$BUILD_DIR/$PROGRAM_NAME" ]]; then
    echo "Error: Compiled Linux program not found in '$BUILD_DIR'."
    echo "Run 'build_linux.sh' first to compile it."
    exit 1
fi

# === Check SSH Connection ===
echo "Checking SSH connection to $REMOTE_IP as $REMOTE_USER..."
sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "$REMOTE_USER@$REMOTE_IP" exit

if [[ $? -ne 0 ]]; then
    echo "Error: Unable to connect to $REMOTE_IP via SSH."
    echo "Ensure SSH is enabled and credentials are correct."
    exit 1
fi

# === Ensure Remote Directory Exists ===
echo "Ensuring remote directory exists: $REMOTE_PATH ..."
sshpass -p "$REMOTE_PASS" ssh -o StrictHostKeyChecking=no "$REMOTE_USER@$REMOTE_IP" "mkdir -p \"$REMOTE_PATH\""

# === Transfer Compiled Program ===
echo "Transferring '$PROGRAM_NAME' to $REMOTE_USER@$REMOTE_IP:$REMOTE_PATH ..."
sshpass -p "$REMOTE_PASS" scp -o StrictHostKeyChecking=no "$BUILD_DIR/$PROGRAM_NAME" "$REMOTE_USER@$REMOTE_IP:$REMOTE_PATH"

if [[ $? -ne 0 ]]; then
    echo "Error: File transfer failed."
    exit 1
fi

echo "Installation complete. The program is now on $REMOTE_IP at $REMOTE_PATH/$PROGRAM_NAME."
exit 0

