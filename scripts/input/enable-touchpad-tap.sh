#!/usr/bin/env bash
set -e

########################################
# Detect Session Type (X11 vs Wayland)
########################################

SESSION_TYPE=${XDG_SESSION_TYPE:-unknown}

echo "Detected session type: $SESSION_TYPE"

########################################
# Enable Tap-to-Click for X11
########################################
enable_x11_tap() {
    echo "Configuring tap-to-click for X11..."
    DEVICE_ID=$(xinput list | grep -i "touchpad" | grep -o 'id=[0-9]*' | cut -d= -f2 | head -n1)

    if [[ -z "$DEVICE_ID" ]]; then
        echo "⚠️ No touchpad device found via xinput."
        exit 1
    fi

    echo "Using device ID: $DEVICE_ID"
    xinput set-prop "$DEVICE_ID" "libinput Tapping Enabled" 1 || {
        echo "⚠️ Failed to enable tapping via xinput."
        exit 1
    }

    echo "✅ Tap-to-click enabled for X11 (device $DEVICE_ID)."
}

########################################
# Enable Tap-to-Click for Wayland
########################################
enable_wayland_tap() {
    echo "Configuring tap-to-click for Wayland..."
    # GNOME / KDE / others rely on libinput settings
    # Persistent config via Xorg.conf.d still applies for XWayland apps

    CONFIG_DIR="/etc/X11/xorg.conf.d"
    CONFIG_FILE="$CONFIG_DIR/99-touchpad-tap.conf"

    sudo mkdir -p "$CONFIG_DIR"
    sudo tee "$CONFIG_FILE" >/dev/null <<EOF
Section "InputClass"
    Identifier "touchpad"
    MatchDriver "libinput"
    Option "Tapping" "on"
EndSection
EOF

    echo "✅ Tap-to-click enabled persistently via $CONFIG_FILE."
    echo "👉 Restart your session or reboot for changes to take effect."
}

########################################
# Main Logic
########################################
case "$SESSION_TYPE" in
    x11)
        enable_x11_tap
        ;;
    wayland)
        enable_wayland_tap
        ;;
    *)
        echo "⚠️ Unknown session type. Please check XDG_SESSION_TYPE."
        ;;
esac

