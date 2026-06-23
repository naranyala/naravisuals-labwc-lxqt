#!/usr/bin/env bash
# Build AppImage for each GUI app (or a combined one).
# Requires: linuxdeploy (with Qt6 plugin)
#
# Usage: ./build-appimage.sh [app-name]
#   app-name: specific app dir to build (e.g. desktop-gui, theme-manager)
#            If omitted, builds ALL apps into individual AppImages.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/appimage"
APPS_DIR="$SCRIPT_DIR/apps"
DEPLOY="${LINUXDEPLOY:-linuxdeploy}"
APPDIR="$BUILD_DIR/AppDir"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

check_deps() {
    if ! command -v "$DEPLOY" &>/dev/null; then
        info "linuxdeploy not found. Installing..."
        local url="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        wget -q "$url" -O /tmp/linuxdeploy
        chmod +x /tmp/linuxdeploy
        DEPLOY=/tmp/linuxdeploy
    fi
    if ! command -v "$DEPLOY" --plugin qt &>/dev/null 2>&1; then
        # Download Qt plugin
        local url="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
        wget -q "$url" -O /tmp/linuxdeploy-plugin-qt
        chmod +x /tmp/linuxdeploy-plugin-qt
        export LINUXDEPLOY_PLUGIN_QT=/tmp/linuxdeploy-plugin-qt
    fi
}

build_single() {
    local app_dir="$1"
    local app_name
    app_name="$(basename "$app_dir" | sed 's/-gui$//; s/-manager$//')"
    local binary_name="nv-$(basename "$app_dir")"

    info "Building AppImage for $binary_name..."

    build_dir="$BUILD_DIR/$app_name"
    mkdir -p "$build_dir"

    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -S "$app_dir" \
        -B "$build_dir" 2>&1 | tail -1

    ninja -C "$build_dir" 2>&1 | tail -1
    DESTDIR="$APPDIR" ninja -C "$build_dir" install 2>&1 | tail -1

    # Create .desktop file for the app
    mkdir -p "$APPDIR/usr/share/applications"
    cat > "$APPDIR/usr/share/applications/$binary_name.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=${binary_name}
Exec=${binary_name}
Icon=${binary_name}
Categories=Settings;X-LXQt;X-Labwc;
Terminal=false
EOF

    # Create icon
    mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
    # Use a built-in fallback icon
    for svg in /usr/share/icons/*/scalable/apps/*.svg; do
        if [ -f "$svg" ]; then
            cp "$svg" "$APPDIR/usr/share/icons/hicolor/256x256/apps/$binary_name.svg" 2>/dev/null || true
            break
        fi
    done

    # Run linuxdeploy
    export LDAI_OUTPUT="$BUILD_DIR/${binary_name}-x86_64.AppImage"
    "$DEPLOY" \
        --appdir "$APPDIR" \
        --plugin qt \
        --output appimage \
        2>&1

    if [ -f "$LDAI_OUTPUT" ]; then
        mv "$LDAI_OUTPUT" "$SCRIPT_DIR/${binary_name}-x86_64.AppImage"
        info "Created: $SCRIPT_DIR/${binary_name}-x86_64.AppImage"
    fi

    rm -rf "$APPDIR"
}

TARGET="${1:-}"

check_deps

if [ -n "$TARGET" ]; then
    if [ -d "$APPS_DIR/$TARGET" ]; then
        build_single "$APPS_DIR/$TARGET"
    else
        error "App directory '$TARGET' not found in $APPS_DIR"
        exit 1
    fi
else
    for d in "$APPS_DIR"/*/; do
        name="$(basename "$d")"
        [ "$name" = "shared" ] && continue
        [ ! -f "$d/CMakeLists.txt" ] && continue
        build_single "$d"
    done
fi

info "All AppImages built successfully."
