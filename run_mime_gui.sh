#!/bin/bash
# Script to build and run the MIME Types GUI

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$SCRIPT_DIR/apps/mime-gui"
BUILD_DIR="$APP_DIR/build"
EXECUTABLE="$BUILD_DIR/nv-mime-gui"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR" || exit 1
cmake .. > /dev/null 2>&1
make -j$(nproc) > /dev/null 2>&1

if [ -x "$EXECUTABLE" ]; then
    "$EXECUTABLE" "$@"
else
    echo "Error: Failed to build or find the executable."
    exit 1
fi
