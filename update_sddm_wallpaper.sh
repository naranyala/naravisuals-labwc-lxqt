#!/bin/bash

# Script to update SDDM login manager background
# Usage: ./update_sddm_wallpaper.sh /path/to/wallpaper.jpg

set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <path-to-wallpaper-image>"
    exit 1
fi

WALLPAPER_SRC="$1"

if [ ! -f "$WALLPAPER_SRC" ]; then
    echo "Error: File '$WALLPAPER_SRC' not found."
    exit 1
fi

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script as root or using sudo."
    exec sudo "$0" "$@"
    exit $?
fi

# Determine the theme to update. 
# We'll use 'ubuntu-budgie-login' as it is the installed and default theme for this setup.
THEME_NAME="ubuntu-budgie-login"
THEME_DIR="/usr/share/sddm/themes/$THEME_NAME"
THEME_CONF="$THEME_DIR/theme.conf"

if [ ! -d "$THEME_DIR" ]; then
    echo "Error: Theme directory '$THEME_DIR' does not exist."
    echo "This script is tailored for the ubuntu-budgie-login theme. Please update the THEME_NAME variable in this script."
    exit 1
fi

# Copy wallpaper to a universally readable location
DEST_WALLPAPER="/usr/share/backgrounds/sddm-custom-wallpaper.jpg"

echo "Copying '$WALLPAPER_SRC' to '$DEST_WALLPAPER'..."
cp "$WALLPAPER_SRC" "$DEST_WALLPAPER"
chmod 644 "$DEST_WALLPAPER"

# Update theme.conf
echo "Updating SDDM theme configuration in $THEME_CONF..."

# Backup the original theme.conf if it hasn't been backed up
if [ ! -f "${THEME_CONF}.bak" ]; then
    cp "$THEME_CONF" "${THEME_CONF}.bak"
    echo "Created backup of original theme.conf at ${THEME_CONF}.bak"
fi

# Update background properties in theme.conf using sed
sed -i "s|^background = .*|background = \"$DEST_WALLPAPER\"|" "$THEME_CONF"

# Also ensure SDDM uses this theme in /etc/sddm.conf
SDDM_CONF="/etc/sddm.conf"
if [ -f "$SDDM_CONF" ]; then
    echo "Updating $SDDM_CONF to ensure Current=$THEME_NAME is set..."
    # Replace Current=... with Current=ubuntu-budgie-login
    if grep -q "^Current=" "$SDDM_CONF"; then
        sed -i "s/^Current=.*/Current=$THEME_NAME/" "$SDDM_CONF"
    else
        sed -i "/^\[Theme\]/a Current=$THEME_NAME" "$SDDM_CONF"
    fi
else
    echo "Creating basic /etc/sddm.conf..."
    echo -e "[Theme]\nCurrent=$THEME_NAME" > "$SDDM_CONF"
fi

echo ""
echo "Success! The SDDM background has been updated."
echo "You can preview it by logging out or restarting your computer."
echo "To revert, restore the backup: sudo cp ${THEME_CONF}.bak ${THEME_CONF}"
