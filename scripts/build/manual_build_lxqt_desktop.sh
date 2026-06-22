#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Dynamically get the root workspace directory
WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Define target versions exactly as requested
QT_VERSION="6.10.2"
QT_TARBALL="qt-everywhere-src-${QT_VERSION}.tar.xz"
QT_URL="https://download.qt.io/official_releases/qt/6.10/${QT_VERSION}/single/${QT_TARBALL}"

LXQT_BT_VER="2.3.0"
LIBLXQT_VER="2.3.0"
LXQT_PANEL_VER="2.3.1"

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

        echo "==> Installing build tools and build dependencies..."
        sudo apt-get install -y build-essential cmake ninja-build wget pkg-config labwc swaybg swayidle swaylock kanshi dunst breeze-icon-theme breeze-cursor-theme sddm emacs-pgtk openbox kwin-x11 kwin-wayland
        sudo apt-get build-dep -y -o Dpkg::Options::='--force-overwrite' qt6-base liblxqt lxqt-panel
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
if [ ! -f "/usr/local/qt6-lightweight/bin/qmake" ]; then
    if [ ! -f "$QT_TARBALL" ]; then
        echo "WARNING: The Qt6 source code tarball ($QT_TARBALL) is missing."
        echo "It is approximately 1.3GB in size. You can download it manually from: $QT_URL"
        read -p "Would you like this script to download it for you right now? [y/N]: " download_choice
        if [[ "$download_choice" =~ ^[Yy]$ ]]; then
            wget -c "$QT_URL"
        else
            echo "Skipping Qt6 compilation. Relying on system Qt6 packages instead."
            SKIP_QT=1
        fi
    fi
    if [ -z "$SKIP_QT" ]; then
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
    fi
else
    echo "Qt6 is already installed at /usr/local/qt6-lightweight. Skipping Qt6 build."
fi

# Export paths so the newly built Qt6 is found by CMake for the next steps
export PATH="/usr/local/qt6-lightweight/bin:$PATH"
export CMAKE_PREFIX_PATH="/usr/local/qt6-lightweight:$CMAKE_PREFIX_PATH"

# ==========================================
# 2. Build lxqt-build-tools
# ==========================================
echo "==> Downloading and building lxqt-build-tools ($LXQT_BT_VER)..."
cd "$WORKSPACE_DIR"
if [ ! -f "lxqt-build-tools-${LXQT_BT_VER}.tar.xz" ]; then
    wget -c "https://github.com/lxqt/lxqt-build-tools/releases/download/${LXQT_BT_VER}/lxqt-build-tools-${LXQT_BT_VER}.tar.xz"
fi
if [ ! -d "lxqt-build-tools-${LXQT_BT_VER}" ]; then
    tar -xf "lxqt-build-tools-${LXQT_BT_VER}.tar.xz"
fi
cd "lxqt-build-tools-${LXQT_BT_VER}"
cmake -B build
make -C build -j$(nproc)
sudo make -C build install

# ==========================================
# 3. Build liblxqt
# ==========================================
echo "==> Downloading and building liblxqt ($LIBLXQT_VER)..."
cd "$WORKSPACE_DIR"
if [ ! -f "liblxqt-${LIBLXQT_VER}.tar.xz" ]; then
    wget -c "https://github.com/lxqt/liblxqt/releases/download/${LIBLXQT_VER}/liblxqt-${LIBLXQT_VER}.tar.xz"
fi
if [ ! -d "liblxqt-${LIBLXQT_VER}" ]; then
    tar -xf "liblxqt-${LIBLXQT_VER}.tar.xz"
fi
cd "liblxqt-${LIBLXQT_VER}"
cmake -B build
make -C build -j$(nproc)
sudo make -C build install

# Update linker cache so the system finds the newly installed liblxqt.so
sudo ldconfig

# ==========================================
# 4. Build lxqt-panel
# ==========================================
echo "==> Downloading and building lxqt-panel ($LXQT_PANEL_VER)..."
cd "$WORKSPACE_DIR"
if [ ! -f "lxqt-panel-${LXQT_PANEL_VER}.tar.xz" ]; then
    wget -c "https://github.com/lxqt/lxqt-panel/releases/download/${LXQT_PANEL_VER}/lxqt-panel-${LXQT_PANEL_VER}.tar.xz"
fi
if [ ! -d "lxqt-panel-${LXQT_PANEL_VER}" ]; then
    tar -xf "lxqt-panel-${LXQT_PANEL_VER}.tar.xz"
fi
cd "lxqt-panel-${LXQT_PANEL_VER}"
cmake -B build
make -C build -j$(nproc)
sudo make -C build install

echo "==> EVERYTHING INSTALLED SUCCESSFULLY!"
lxqt-panel --version
