#!/usr/bin/env bash
# enable-natural-scrolling.sh
# Enables natural scrolling for touchpads on Linux (X11 & Wayland)

set -e

SESSION_TYPE=${XDG_SESSION_TYPE:-x11}

enable_x11() {
    echo "Detected X11 session..."
    # Find touchpad device
    device=$(xinput list | grep -i touchpad | grep -o 'id=[0-9]*' | cut -d= -f2 | head -n1)
    if [ -n "$device" ]; then
        echo "Enabling natural scrolling on device ID $device"
        xinput set-prop "$device" "libinput Natural Scrolling Enabled" 1
    else
        echo "No touchpad device found!"
    fi
}

enable_wayland() {
    echo "Detected Wayland session..."
    # Persistent config for libinput via udev hwdb
    sudo bash -c 'cat > /etc/libinput/local-overrides.quirks <<EOF
[Touchpad Override]
MatchUdevType=touchpad
MatchName=*
AttrNaturalScrolling=1
EOF'
    echo "Natural scrolling enabled via libinput override."
    echo "Reboot or re-login required for changes to take effect."
}

case "$SESSION_TYPE" in
    x11)
        enable_x11
        ;;
    wayland)
        enable_wayland
        ;;
    *)
        echo "Unknown session type: $SESSION_TYPE"
        ;;
esac

