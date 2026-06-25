#!/usr/bin/env bash
# Build all GUI apps and generate distribution packages (deb/rpm/txz)
# Usage: ./build-all.sh [--deb|--rpm|--txz|--appimage]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/release"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()    { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
section() { echo; echo "============================================"; echo " $*"; echo "============================================"; }

check_prereqs() {
    local missing=()
    command -v cmake    &>/dev/null || missing+=("cmake")
    command -v ninja    &>/dev/null || missing+=("ninja")
    command -v c++      &>/dev/null || missing+=("g++")
    command -v pkg-config &>/dev/null && {
        pkg-config --exists Qt6Core  2>/dev/null || warn "Qt6Core development package not found (install qt6-base-dev)"
    } || warn "pkg-config not found"

    if [ ${#missing[@]} -gt 0 ]; then
        error "Missing prerequisites: ${missing[*]}"
        exit 1
    fi
}

build_all() {
    section "Configuring CMake"
    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -S "$SCRIPT_DIR" \
        -B "$BUILD_DIR" \
        -DCMAKE_SKIP_RPATH=ON

    section "Building all GUI apps"
    ninja -C "$BUILD_DIR" -j"$(nproc)"

    section "Installing to staging directory"
    DESTDIR="$BUILD_DIR/stage" ninja -C "$BUILD_DIR" install
    info "Binaries staged at: $BUILD_DIR/stage/usr/bin/"
    ls -1 "$BUILD_DIR/stage/usr/bin/" 2>/dev/null || warn "No binaries installed"
}

package_deb() {
    section "Building .deb package"
    (cd "$BUILD_DIR" && cpack -G DEB 2>&1)
    mkdir -p "$SCRIPT_DIR/dist"
    find "$BUILD_DIR" -name "*.deb" -exec cp {} "$SCRIPT_DIR/dist/" \;
    info "DEB packages in dist/:"
    ls -1 "$SCRIPT_DIR/dist/"*.deb 2>/dev/null || warn "No .deb generated"
}

package_rpm() {
    section "Building .rpm package"
    (cd "$BUILD_DIR" && cpack -G RPM 2>&1) || warn "RPM build failed (may need rpmbuild)"
    find "$BUILD_DIR" -name "*.rpm" -exec cp {} "$SCRIPT_DIR/dist/" \; 2>/dev/null || true
    info "RPM packages in dist/"
}

package_txz() {
    section "Building source archive (.tar.xz)"
    (cd "$BUILD_DIR" && cpack -G TXZ 2>&1)
    find "$BUILD_DIR" -name "*.tar.*" -exec cp {} "$SCRIPT_DIR/dist/" \;
    info "Archives in dist/"
}

package_appimage() {
    section "Building AppImages"
    if [ -f "$SCRIPT_DIR/build-appimage.sh" ]; then
        bash "$SCRIPT_DIR/build-appimage.sh"
        mv -f "$SCRIPT_DIR"/*.AppImage "$SCRIPT_DIR/dist/" 2>/dev/null || true
    else
        warn "build-appimage.sh not found"
    fi
}

# --- Main ---
cd "$SCRIPT_DIR"
check_prereqs

FORMATS=("${@:-deb rpm txz}")
BUILD_DEB=false
BUILD_RPM=false
BUILD_TXZ=false
BUILD_APPIMAGE=false

for arg in "$@"; do
    case "$arg" in
        --deb|-d)       BUILD_DEB=true ;;
        --rpm|-r)       BUILD_RPM=true ;;
        --txz|-t)       BUILD_TXZ=true ;;
        --appimage|-a)  BUILD_APPIMAGE=true ;;
        --all)          BUILD_DEB=true; BUILD_RPM=true; BUILD_TXZ=true; BUILD_APPIMAGE=true ;;
        --help|-h)
            echo "Usage: $0 [--deb|--rpm|--txz|--appimage|--all]"
            echo "Default (no args): deb rpm txz"
            exit 0 ;;
        *)              warn "Unknown option: $arg" ;;
    esac
done

if [ $# -eq 0 ]; then
    BUILD_DEB=true
    BUILD_RPM=true
    BUILD_TXZ=true
fi

mkdir -p "$SCRIPT_DIR/dist"

build_all

$BUILD_DEB && package_deb
$BUILD_RPM && package_rpm
$BUILD_TXZ && package_txz
$BUILD_APPIMAGE && package_appimage

section "Build Complete"
info "Distribution packages:"
ls -lh "$SCRIPT_DIR/dist/" 2>/dev/null || echo "(none)"
