#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WALLPAPER="$(realpath "$SCRIPT_DIR/../wallpaper.jpg")"

# Ensure the config directory exists
mkdir -p "$HOME/.config/labwc"

# Set the active wallpaper path
echo "$WALLPAPER" > "$HOME/.config/labwc/wallpaper"
echo "Active wallpaper set to: $WALLPAPER"

# Reload swaybg if it is running
if pgrep swaybg &>/dev/null; then
    pkill swaybg 2>/dev/null || true
    swaybg -i "$WALLPAPER" -m fill &>/dev/null &
    echo "swaybg reloaded with new wallpaper."
else
    # Start swaybg if it's not running
    swaybg -i "$WALLPAPER" -m fill &>/dev/null &
    echo "Started swaybg with new wallpaper."
fi
