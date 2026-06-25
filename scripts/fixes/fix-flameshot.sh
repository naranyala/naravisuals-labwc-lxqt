#!/bin/bash

CONFIG_FILE="$HOME/.config/flameshot/flameshot.ini"
CONFIG_DIR="$HOME/.config/flameshot"

# 1. Check if Flameshot is installed
if ! command -v flameshot &> /dev/null; then
    echo "Error: Flameshot is not installed on this system."
    exit 1
fi

# 2. Extract the major version number
VERSION_STR=$(flameshot --version 2>&1 | head -n 1)
MAJOR_VERSION=$(echo "$VERSION_STR" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | cut -d. -f1)

if [ -z "$MAJOR_VERSION" ]; then
    echo "Could not detect Flameshot version. Proceeding cautiously..."
    MAJOR_VERSION=0
fi

echo "Detected Flameshot Major Version: $MAJOR_VERSION"

# 3. Create config directory/file if they do not exist
if [ ! -d "$CONFIG_DIR" ]; then
    mkdir -p "$CONFIG_DIR"
fi

if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "[General]\n" > "$CONFIG_FILE"
fi

# 4. Apply fix based on version
if [ "$MAJOR_VERSION" -ge 14 ]; then
    echo "Flameshot v14+ detected. Stripping deprecated 'useGrimAdapter' config..."

    # Remove any line containing useGrimAdapter or disabledGrimWarning
    sed -i '/useGrimAdapter/d' "$CONFIG_FILE"
    sed -i '/disabledGrimWarning/d' "$CONFIG_FILE"

    echo "Configuration cleaned. Flameshot v14 native portals will be used."

else
    echo "Flameshot v13 (or older) detected. Injecting grim adapter variables..."

    # Ensure [General] section exists
    if ! grep -q "\[General\]" "$CONFIG_FILE"; then
        echo -e "\n[General]" >> "$CONFIG_FILE"
    fi

    # Remove existing variants to avoid duplicates
    sed -i '/useGrimAdapter/d' "$CONFIG_FILE"
    sed -i '/disabledGrimWarning/d' "$CONFIG_FILE"

    # Append the required configurations under [General]
    sed -i '/\[General\]/a useGrimAdapter=true\ndisabledGrimWarning=true' "$CONFIG_FILE"

    echo "Grim adapter forced successfully."
fi

echo "Done! Try launching 'flameshot gui' now."

