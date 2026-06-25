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

# Pidfile to prevent duplicate processes
PIDFILE="${XDG_RUNTIME_DIR:-/tmp}/wl-clipboard-persistence.pid"

# Kill any old clipboard persistence process
if [ -f "$PIDFILE" ]; then
    kill "$(cat "$PIDFILE")" 2>/dev/null || true
    rm -f "$PIDFILE"
fi
pkill -f "wl-paste --watch" 2>/dev/null || true

# Start clipboard persistence in background
nohup wl-paste --watch wl-copy &>/dev/null &
echo $! > "$PIDFILE"

echo "✅ Wayland clipboard persistence enabled (PID: $!)."
echo "You can now copy-paste reliably across apps."

