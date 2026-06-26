#!/bin/bash
# Launch nv-dock-gui — Crystal Dock pinned apps manager

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/apps/dock-gui/build"

if [ ! -f "$BUILD_DIR/nv-dock-gui" ]; then
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && cmake "$PROJECT_ROOT/apps/dock-gui" && cmake --build .
fi

exec "$BUILD_DIR/nv-dock-gui" "$@"
