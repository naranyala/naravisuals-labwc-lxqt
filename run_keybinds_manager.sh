#!/bin/bash
# Script to build and run the Global Keybindings Manager GUI

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$SCRIPT_DIR/apps/keybinds-gui"
BUILD_DIR="$APP_DIR/build"
EXECUTABLE="$BUILD_DIR/nv-keybinds-gui"

echo "=========================================="
echo "    Starting Global Keybindings Manager   "
echo "=========================================="

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

# Build the application to ensure it's up to date
echo "Building the application..."
cd "$BUILD_DIR" || exit 1
cmake .. > /dev/null 2>&1
make -j$(nproc) > /dev/null 2>&1

# Check if build was successful and run it
if [ -x "$EXECUTABLE" ]; then
    echo "Launching GUI..."
    "$EXECUTABLE" "$@"
else
    echo "Error: Failed to build or find the Keybinds Manager executable at $EXECUTABLE"
    exit 1
fi
