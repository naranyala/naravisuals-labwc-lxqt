#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/apps/bluetooth-gui/build"
if [ ! -f "$BUILD_DIR/nv-bluetooth-gui" ]; then
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && cmake -GNinja .. && ninja && cd "$SCRIPT_DIR"
fi
exec "$BUILD_DIR/nv-bluetooth-gui" "$@"
