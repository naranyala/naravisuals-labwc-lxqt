#!/bin/bash

# Get current user
# USER_NAME=$(whoami)
USER_NAME="naranyala"
# MOUNT_BASE="/run/media/$USER_NAME"
MOUNT_BASE="/media/$USER_NAME"

# Create mount base if it doesn't exist
mkdir -p "$MOUNT_BASE"

# Scan all partitions (excluding loop devices)
echo "🔍 Scanning available partitions..."
PARTITIONS=($(lsblk -lnpo NAME,TYPE | grep 'part' | awk '{print $1}'))

# Display list of partitions
echo "📋 Available partitions:"
for i in "${!PARTITIONS[@]}"; do
    PART="${PARTITIONS[$i]}"
    FS_TYPE=$(lsblk -no FSTYPE "$PART")
    SIZE=$(lsblk -no SIZE "$PART")
    LABEL=$(blkid -s LABEL -o value "$PART")
    echo "$i) $PART — ${LABEL:-NoLabel} — $FS_TYPE — $SIZE"
done

# Ask user to pick a partition
read -p "👉 Enter the number of the partition to mount: " CHOICE

# Validate input
if [[ ! "$CHOICE" =~ ^[0-9]+$ ]] || [ "$CHOICE" -ge "${#PARTITIONS[@]}" ]; then
    echo "❌ Invalid choice. Exiting."
    exit 1
fi

# Get selected partition
SELECTED_PART="${PARTITIONS[$CHOICE]}"
LABEL=$(blkid -s LABEL -o value "$SELECTED_PART")
UUID=$(blkid -s UUID -o value "$SELECTED_PART")
DISK_NAME="${LABEL:-$UUID}"
MOUNT_POINT="$MOUNT_BASE/$DISK_NAME"

# Create mount point
mkdir -p "$MOUNT_POINT"

# Mount the partition
echo "📁 Mounting $SELECTED_PART to $MOUNT_POINT..."
mount "$SELECTED_PART" "$MOUNT_POINT"

if [ $? -eq 0 ]; then
    echo "✅ Successfully mounted at $MOUNT_POINT"
else
    echo "❌ Failed to mount $SELECTED_PART"
fi

