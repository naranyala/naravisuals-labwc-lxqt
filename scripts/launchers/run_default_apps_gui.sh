#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$SCRIPT_DIR/../../CMakeLists.txt" ]; then
    PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
elif [ -f "$SCRIPT_DIR/CMakeLists.txt" ]; then
    PROJECT_ROOT="$SCRIPT_DIR"
else
    PROJECT_ROOT="$(pwd)"
fi
BUILD_DIR="$PROJECT_ROOT/apps/default-apps-gui/build"

if [ ! -f "$BUILD_DIR/nv-default-apps-gui" ]; then
    echo "Building nv-default-apps-gui..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR" || exit 1
    cmake -GNinja .. && ninja
fi

exec "$BUILD_DIR/nv-default-apps-gui" "$@"
