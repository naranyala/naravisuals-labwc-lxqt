#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/../.."
BUILD_DIR="$PROJECT_ROOT/apps/panel-gui/build"
if [ ! -f "$BUILD_DIR/nv-panel-gui" ]; then
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && cmake "$PROJECT_ROOT/apps/panel-gui" && cmake --build .
fi
exec "$BUILD_DIR/nv-panel-gui" "$@"
