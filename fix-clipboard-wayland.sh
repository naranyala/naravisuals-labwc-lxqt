#!/usr/bin/env bash
# Wayland clipboard fix script
# Ensures wl-clipboard tools are installed and runs a persistence daemon

set -e

# Check if wl-clipboard is installed
if ! command -v wl-copy >/dev/null 2>&1 || ! command -v wl-paste >/dev/null 2>&1; then
    echo "Installing wl-clipboard..."
    if command -v apt >/dev/null 2>&1; then
        sudo apt update && sudo apt install -y wl-clipboard
    elif command -v pacman >/dev/null 2>&1; then
        sudo pacman -S --noconfirm wl-clipboard
    elif command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y wl-clipboard
    else
        echo "❌ Package manager not detected. Install wl-clipboard manually."
        exit 1
    fi
fi

# Kill any old clipboard persistence process
pkill -f "wl-paste --watch" || true

# Start clipboard persistence in background
nohup wl-paste --watch wl-copy &>/dev/null &

echo "✅ Wayland clipboard persistence enabled."
echo "You can now copy-paste reliably across apps."

