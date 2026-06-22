#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Dynamically get the directory where this script is located
WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Define target versions
QT_VERSION="6.10.2"
QT_TARBALL="qt-everywhere-src-${QT_VERSION}.tar.xz"
QT_URL="https://download.qt.io/official_releases/qt/6.10/${QT_VERSION}/single/${QT_TARBALL}"

LXQT_REPO_URL="https://github.com/lxqt/lxqt.git"

install_deps() {
    if command -v dnf &>/dev/null; then
        echo "==> RPM-based system detected. Using dnf..."
        sudo dnf install -y epel-release
        sudo dnf config-manager --set-enabled crb || sudo dnf config-manager --set-enabled powertools || true
        sudo dnf groupinstall -y "Development Tools"
        sudo dnf install -y cmake ninja-build wget pkgconf git \
            labwc swaybg swayidle swaylock kanshi dunst breeze-icon-theme breeze-cursor-theme \
            sddm emacs openbox kwin-x11 kwin-wayland
        
        echo "==> Installing build dependencies for LXQt components..."
        sudo dnf install -y kf6-kwindowsystem-devel kf6-solid-devel kf6-kcoreaddons-devel \
            polkit-qt6-1-devel qt6-qtsvg-devel libdbusmenu-glib-devel \
            xorg-x11-server-devel libxcb-devel xcb-util-devel xcb-util-keysyms-devel xcb-util-wm-devel \
            qt6-qtbase-devel qt6-qttools-devel \
            glib2-devel libX11-devel libX11-xcb-devel
            
    elif command -v apt-get &>/dev/null; then
        echo "==> DEB-based system detected. Using apt-get..."
        echo "==> Enabling deb-src repositories..."
        if [ -f /etc/apt/sources.list.d/ubuntu.sources ] && grep -q "^Types: deb$" /etc/apt/sources.list.d/ubuntu.sources; then
            sudo sed -i 's/^Types: deb$/Types: deb deb-src/' /etc/apt/sources.list.d/ubuntu.sources
        elif [ -f /etc/apt/sources.list ] && grep -q "^# deb-src" /etc/apt/sources.list; then
            sudo sed -i '/^#\s*deb-src/s/^#\s*//' /etc/apt/sources.list
        fi

        echo "==> Updating apt cache..."
        sudo apt-get update

        echo "==> Fixing any broken dependencies..."
        sudo apt-get install -f -y -o Dpkg::Options::='--force-overwrite'

        echo "==> Installing build tools and basic dependencies..."
        sudo apt-get install -y build-essential cmake ninja-build wget pkg-config git \
            labwc swaybg swayidle swaylock kanshi dunst breeze-icon-theme breeze-cursor-theme \
            sddm emacs-pgtk openbox kwin-x11 kwin-wayland

        echo "==> Fetching build dependencies for LXQt components..."
        # We get build-dep for various LXQt components to ensure we have libraries like KF6WindowSystem, PolkitQt6, etc.
        sudo apt-get build-dep -y -o Dpkg::Options::='--force-overwrite' \
            qt6-base liblxqt lxqt-panel pcmanfm-qt lxqt-session lxqt-runner \
            lxqt-config lxqt-powermanagement || true

        # Provide some known Qt6/KF6 development packages explicitly in case the apt build-deps are for older Qt5 versions
        sudo apt-get install -y libkf6windowsystem-dev libkf6solid-dev libkf6coreaddons-dev \
            libpolkit-qt6-1-dev libqt6svg6-dev libdbusmenu-glib-dev xorg-dev libx11-xcb-dev \
            libxcb-util0-dev libxcb-randr0-dev || true
    else
        echo "Unsupported package manager. Please install dependencies manually."
        exit 1
    fi
}

install_deps

# ==========================================
# 1. Build a Lightweight Qt6
# ==========================================
echo "==> Downloading and building lightweight Qt6 ($QT_VERSION)..."
cd "$WORKSPACE_DIR"
if [ ! -d "qt6-build" ]; then
    if [ ! -f "$QT_TARBALL" ]; then
        wget -c "$QT_URL"
    fi
    if [ ! -d "qt-everywhere-src-${QT_VERSION}" ]; then
        tar -xf "$QT_TARBALL"
    fi
    mkdir -p qt6-build
    cd qt6-build

    # Configure Qt for a lightweight build
    ../qt-everywhere-src-${QT_VERSION}/configure -release -nomake examples -nomake tests -opensource -confirm-license \
        -prefix /usr/local/qt6-lightweight \
        -skip qtwebengine -skip qt3d -skip qtmultimedia -skip qtdeclarative \
        -make libs

    cmake --build . --parallel $(nproc)
    sudo cmake --install .
else
    echo "Qt6 build directory already exists. Skipping Qt6 build."
fi

# Export paths so the newly built Qt6 is found by CMake for the next steps
export PATH="/usr/local/qt6-lightweight/bin:$PATH"
export CMAKE_PREFIX_PATH="/usr/local/qt6-lightweight:$CMAKE_PREFIX_PATH"

# ==========================================
# 2. Clone latest LXQt source from GitHub
# ==========================================
echo "==> Cloning latest LXQt from GitHub..."
cd "$WORKSPACE_DIR"
if [ ! -d "lxqt" ]; then
    # We use --recursive to pull all submodules which represent individual LXQt components
    git clone --recursive "$LXQT_REPO_URL"
else
    echo "LXQt directory exists. Updating..."
    cd lxqt
    git pull
    git submodule update --init --recursive
    cd "$WORKSPACE_DIR"
fi

# ==========================================
# 3. Build all LXQt components from source
# ==========================================
echo "==> Building all LXQt components..."
cd "$WORKSPACE_DIR/lxqt"

# Set environment variables for the build_all_cmake_projects.sh script
export CMAKE_GENERATOR="Ninja"
export DO_BUILD=1
export DO_INSTALL=1
export DO_INSTALL_ROOT=1

# Execute the LXQt provided script to build and install everything in order
./build_all_cmake_projects.sh

echo "==> EVERYTHING INSTALLED SUCCESSFULLY!"
lxqt-session --version
lxqt-panel --version
