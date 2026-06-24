#!/usr/bin/env bash
# Toggle mute/unmute for audio input/output devices
# Works with PipeWire (wpctl) or PulseAudio (pactl)

set -e

# Detect backend
if command -v wpctl >/dev/null 2>&1; then
    BACKEND="pipewire"
elif command -v pactl >/dev/null 2>&1; then
    BACKEND="pulseaudio"
else
    echo "❌ No supported audio backend found (wpctl/pactl)."
    exit 1
fi

# Function to toggle mute
toggle_mute() {
    local target="$1"
    if [ "$BACKEND" = "pipewire" ]; then
        # wpctl: list devices with wpctl status
        wpctl set-mute "$target" toggle
    else
        # pactl: list sinks/sources with pactl list short
        pactl set-${2} "$target" toggle
    fi
}

echo "Audio backend detected: $BACKEND"

# Example usage:
# Toggle default output (sink)
if [ "$BACKEND" = "pipewire" ]; then
    toggle_mute @DEFAULT_AUDIO_SINK@
else
    toggle_mute @DEFAULT_SINK@ sink
fi

# Toggle default input (source/mic)
if [ "$BACKEND" = "pipewire" ]; then
    toggle_mute @DEFAULT_AUDIO_SOURCE@
else
    toggle_mute @DEFAULT_SOURCE@ source
fi

echo "✅ Toggled mute/unmute for default input & output."

