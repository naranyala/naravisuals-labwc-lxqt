#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/apps/wallpaper-gui/build"

if [ ! -f "$BUILD_DIR/nv-wallpaper-gui" ]; then
    echo "Building nv-wallpaper-gui..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. && make -j"$(nproc)"
    cd "$SCRIPT_DIR"
fi

exec "$BUILD_DIR/nv-wallpaper-gui" "$@"
