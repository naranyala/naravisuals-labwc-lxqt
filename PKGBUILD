# Maintainer: naranyala
pkgname=naravisuals-labwc-lxqt
pkgver=1.0.0
pkgrel=1
pkgdesc="Comprehensive LXQt + Labwc Wayland desktop environment suite"
arch=('x86_64' 'aarch64')
url="https://github.com/naranyala/naravisuals-labwc-lxqt"
license=('GPL2')
depends=(
    'lxqt'
    'labwc'
    'qt6-base'
    'qt6ct'
    'swaybg'
    'swayidle'
    'swaylock'
    'wl-clipboard'
    'grim'
    'slurp'
    'dunst'
    'kanshi'
    'brightnessctl'
    'playerctl'
    'pavucontrol-qt'
    'breeze-icon-theme'
    'breeze-cursor-theme'
)
optdepends=(
    'rofi-wayland: Application launcher'
    'wob: On-screen display for volume/brightness'
    'cliphist: Clipboard history'
    'xdg-desktop-portal-wlr: Screen sharing'
    'kvantum: SVG-based Qt theming'
    'neofetch: System information display'
    'conky: Desktop system monitor'
    'emacs: Text editor'
)
makedepends=(
    'cmake'
    'ninja'
    'gcc'
)
backup=(
    "etc/xdg/naravisuals/labwc/rc.xml"
    "etc/xdg/naravisuals/labwc/environment"
)
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"

    # Build dotfiles installer
    cd cmd/dotfiles-manager
    c++ -std=c++17 build.cpp -o lxqt-dotfiles
    cd ../..

    # Build Control Center
    cd apps/control-center
    mkdir -p build && cd build
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/usr ..
    ninja
    cd ../../..

    # Build SDDM GUI
    cd apps/sddm-gui
    mkdir -p build && cd build
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=/usr ..
    ninja
    cd ../../..
}

package() {
    cd "$pkgname-$pkgver"

    # Install dotfiles to system-wide XDG location
    install -Dm644 configs/dotfiles/labwc/rc.xml "$pkgdir/etc/xdg/naravisuals/labwc/rc.xml"
    install -Dm644 configs/dotfiles/labwc/environment "$pkgdir/etc/xdg/naravisuals/labwc/environment"
    install -Dm644 configs/dotfiles/labwc/themerc "$pkgdir/etc/xdg/naravisuals/labwc/themerc-override"
    install -Dm755 configs/dotfiles/labwc/autostart "$pkgdir/etc/xdg/naravisuals/labwc/autostart"
    install -Dm644 configs/dotfiles/labwc/menu.xml "$pkgdir/etc/xdg/naravisuals/labwc/menu.xml"
    install -Dm644 configs/dotfiles/labwc/shutdown "$pkgdir/etc/xdg/naravisuals/labwc/shutdown"

    install -Dm644 configs/dotfiles/lxqt/panel.conf "$pkgdir/etc/xdg/naravisuals/lxqt/panel.conf"
    install -Dm644 configs/dotfiles/lxqt/panel-stock.conf "$pkgdir/etc/xdg/naravisuals/lxqt/panel-stock.conf"
    install -Dm644 configs/dotfiles/lxqt/session.conf "$pkgdir/etc/xdg/naravisuals/lxqt/session.conf"
    install -Dm644 configs/dotfiles/lxqt/lxqt.conf "$pkgdir/etc/xdg/naravisuals/lxqt/lxqt.conf"

    install -Dm644 configs/dotfiles/gtk-3.0/settings.ini "$pkgdir/etc/xdg/naravisuals/gtk-3.0/settings.ini"
    install -Dm644 configs/dotfiles/gtk-4.0/settings.ini "$pkgdir/etc/xdg/naravisuals/gtk-4.0/settings.ini"

    install -Dm644 configs/dotfiles/rofi/config.rasi "$pkgdir/etc/xdg/naravisuals/rofi/config.rasi"
    install -Dm644 configs/dotfiles/wob/config "$pkgdir/etc/xdg/naravisuals/wob/config"
    install -Dm644 configs/dotfiles/cliphist/config "$pkgdir/etc/xdg/naravisuals/cliphist/config"
    install -Dm644 configs/dotfiles/dunst/dunstrc "$pkgdir/etc/xdg/naravisuals/dunst/dunstrc"
    install -Dm644 configs/dotfiles/swaylock/config "$pkgdir/etc/xdg/naravisuals/swaylock/config"

    # Install compositor profiles
    install -Dm644 configs/compositors/hyprland/hyprland.conf "$pkgdir/etc/xdg/naravisuals/compositors/hyprland/hyprland.conf"
    install -Dm644 configs/compositors/sway/config "$pkgdir/etc/xdg/naravisuals/compositors/sway/config"
    install -Dm644 configs/compositors/wayfire/wayfire.ini "$pkgdir/etc/xdg/naravisuals/compositors/wayfire/wayfire.ini"

    # Install binaries
    install -Dm755 cmd/dotfiles-manager/lxqt-dotfiles "$pkgdir/usr/bin/nv-dotfiles"
    install -Dm755 apps/control-center/build/nv-control-center "$pkgdir/usr/bin/nv-control-center"
    install -Dm755 apps/sddm-gui/build/nv-sddm-gui "$pkgdir/usr/bin/nv-sddm-gui"
    install -Dm755 apps/audio-gui/build/nv-audio-gui "$pkgdir/usr/bin/nv-audio-gui"
    install -Dm755 apps/display-gui/build/nv-display-gui "$pkgdir/usr/bin/nv-display-gui"
    install -Dm755 apps/wallpaper-gui/build/nv-wallpaper-gui "$pkgdir/usr/bin/nv-wallpaper-gui"
    install -Dm755 apps/input-gui/build/nv-input-gui "$pkgdir/usr/bin/nv-input-gui"
    install -Dm755 apps/power-gui/build/nv-power-gui "$pkgdir/usr/bin/nv-power-gui"
    install -Dm755 apps/bluetooth-gui/build/nv-bluetooth-gui "$pkgdir/usr/bin/nv-bluetooth-gui"
    install -Dm755 apps/network-gui/build/nv-network-gui "$pkgdir/usr/bin/nv-network-gui"
    install -Dm755 apps/service-gui/build/nv-service-gui "$pkgdir/usr/bin/nv-service-gui"
    install -Dm755 apps/log-gui/build/nv-log-gui "$pkgdir/usr/bin/nv-log-gui"
    install -Dm755 apps/disk-gui/build/nv-disk-gui "$pkgdir/usr/bin/nv-disk-gui"
    install -Dm755 apps/theme-gui/build/nv-theme-gui "$pkgdir/usr/bin/nv-theme-gui"

    # Install scripts
    install -Dm755 scripts/reset-panel.sh "$pkgdir/usr/bin/nv-reset-panel"
    install -Dm755 scripts/install-all.sh "$pkgdir/usr/share/naravisuals/scripts/install-all.sh"

    # Install shared library
    install -Dm644 scripts/lib.sh "$pkgdir/usr/share/naravisuals/scripts/lib.sh"

    # Install feature scripts
    for script in scripts/features/*.sh; do
        install -Dm755 "$script" "$pkgdir/usr/share/naravisuals/scripts/$(basename "$script")"
    done

    # Install QSS
    install -Dm644 configs/dotfiles/lxqt/lxqt-panel.qss "$pkgdir/etc/xdg/naravisuals/lxqt/lxqt-panel.qss"

    # Install session files
    install -Dm644 configs/dotfiles/wayland-sessions/lxqt-labwc.desktop "$pkgdir/usr/share/wayland-sessions/lxqt-labwc.desktop"

    # Install SDDM config
    install -Dm644 configs/dotfiles/sddm/lxqt-labwc.conf "$pkgdir/etc/sddm.conf.d/naravisuals-lxqt-labwc.conf"

    # Install wallpapers
    install -Dm644 assets/wallpaper.jpg "$pkgdir/usr/share/naravisuals/wallpapers/wallpaper.jpg"
}
