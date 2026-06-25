#!/bin/bash
# Script to build and run the Environment Variables Configurator GUI

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$SCRIPT_DIR/apps/env-gui"
BUILD_DIR="$APP_DIR/build"
EXECUTABLE="$BUILD_DIR/nv-env-gui"

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
