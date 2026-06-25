#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/apps/autorun-gui/build"
if [ ! -f "$BUILD_DIR/nv-autorun-gui" ]; then
    mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && cmake -GNinja .. && ninja
fi
exec "$BUILD_DIR/nv-autorun-gui" "$@"
