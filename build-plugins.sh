#!/bin/bash
# Naravisuals — Build LXQt Panel Plugins
# ========================================
# Builds and optionally installs all custom panel plugins.
#
# Usage:
#   bash build-plugins.sh              # Build all plugins
#   bash build-plugins.sh --install    # Build and install
#   bash build-plugins.sh --clean      # Clean build directory
#   bash build-plugins.sh <plugin>     # Build specific plugin

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLUGIN_DIR="$SCRIPT_DIR/plugins"
BUILD_DIR="$PLUGIN_DIR/build"
INSTALL=false
CLEAN=false
TARGET=""

PLUGINS=(
  naravisuals-weather-widget
  naravisuals-cpu-monitor
  naravisuals-sticky-notes
  naravisuals-color-picker
  naravisuals-screenshot
  naravisuals-nightlight
  naravisuals-updates
)

for arg in "$@"; do
  case "$arg" in
    --install|-i) INSTALL=true ;;
    --clean|-c) CLEAN=true ;;
    --help|-h)
      printf "Build LXQt Panel Plugins\n\n"
      printf "Usage: bash build-plugins.sh [options] [plugin]\n\n"
      printf "Options:\n"
      printf "  --install, -i    Install after building\n"
      printf "  --clean, -c      Clean build directory\n"
      printf "  --help, -h       Show this help\n\n"
      printf "Plugins:\n"
      for p in "${PLUGINS[@]}"; do
        printf "  %s\n" "$p"
      done
      exit 0
      ;;
    naravisuals-*) TARGET="$arg" ;;
  esac
done

# Check deps
if ! command -v cmake &>/dev/null; then
  echo "Error: cmake not found. Install: sudo apt install cmake"
  exit 1
fi

if ! dpkg -s lxqt-panel-dev &>/dev/null 2>&1 && ! pkg-config --exists lxqt-panel 2>/dev/null; then
  echo "Warning: lxqt-panel dev headers may not be installed."
  echo "Install: sudo apt install lxqt-panel-dev"
fi

# Clean
if [ "$CLEAN" = true ]; then
  echo "Cleaning build directory..."
  rm -rf "$BUILD_DIR"
  echo "Done."
  exit 0
fi

# Build
echo "=== Building LXQt Panel Plugins ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ -n "$TARGET" ]; then
  echo "Building $TARGET..."
  cmake "$PLUGIN_DIR/$TARGET" -GNinja 2>&1 | tail -5
  ninja 2>&1 | tail -5
else
  echo "Building all plugins..."
  cmake "$PLUGIN_DIR" -GNinja 2>&1 | tail -10
  ninja 2>&1 | tail -10
fi

# Install
if [ "$INSTALL" = true ]; then
  echo ""
  echo "Installing plugins..."
  sudo ninja install 2>&1 | tail -5
  echo "Done. Restart lxqt-panel to load new plugins."
else
  echo ""
  echo "Built plugins are in: $BUILD_DIR"
  echo "Run with --install to install to system."
fi
