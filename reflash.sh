#!/bin/bash
set -e

LABEL="AZ3166"
MOUNT_POINT="/mnt/AZ3166"
DEPLOY_SCRIPT="./MXChip/AZ3166/scripts/deploy.sh"

echo "[INFO] Looking for $LABEL..."

DEVICE=$(lsblk -rno PATH,LABEL | awk -v label="$LABEL" '$2 == label {print $1; exit}')

if [ -z "$DEVICE" ]; then
    echo "[ERROR] AZ3166 device not found."
    echo "[ERROR] Make sure the board is attached to WSL with usbipd."
    exit 1
fi

echo "[INFO] Found AZ3166 at $DEVICE"

# Unmount if currently mounted
if findmnt -rn "$MOUNT_POINT" >/dev/null 2>&1; then
    echo "[INFO] Unmounting $MOUNT_POINT..."
    sudo umount "$MOUNT_POINT"
fi

# Repair/check the virtual FAT filesystem
echo "[INFO] Checking filesystem..."
sudo fsck.vfat -a "$DEVICE"

# Ensure mount point exists
sudo mkdir -p "$MOUNT_POINT"

# Mount read/write
echo "[INFO] Mounting $DEVICE at $MOUNT_POINT..."
sudo mount -o rw "$DEVICE" "$MOUNT_POINT"

# Verify that it really mounted read/write
if ! findmnt -rn -o OPTIONS "$MOUNT_POINT" | grep -qw "rw"; then
    echo "[ERROR] AZ3166 did not mount read/write."
    exit 1
fi

echo "[INFO] AZ3166 mounted read/write."

# Deploy
echo "[INFO] Flashing AZ3166..."
sudo "$DEPLOY_SCRIPT" "$MOUNT_POINT"

echo "[OK] AZ3166 reflash complete."
