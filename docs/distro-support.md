# Distro Support

Package names, repositories, and per-distro notes.

## Supported Distributions

| Distro | Package Manager | Status |
|--------|----------------|--------|
| Debian 12+ | apt | Fully supported |
| Ubuntu 22.04+ | apt | Fully supported |
| Fedora 38+ | dnf | Fully supported (with Copr) |
| Rocky/Alma 9+ | dnf | Supported (with EPEL) |
| Arch Linux | pacman | Fully supported |
| Manjaro | pacman | Supported |
| openSUSE Tumbleweed | zypper | Supported |
| Void Linux | xbps | Partial support |
| Alpine Linux | apk | Partial support |

## Package Name Mapping

| Component | apt (Debian/Ubuntu) | dnf (Fedora/RHEL) | pacman (Arch) |
|-----------|--------------------|--------------------|---------------|
| Build tools | build-essential | "Development Tools" group | base-devel |
| C++ compiler | g++ | gcc-c++ | gcc |
| CMake | cmake | cmake | cmake |
| Ninja | ninja-build | ninja-build | ninja |
| Qt6 base dev | qt6-base-dev | qt6-qtbase-devel | qt6-base |
| Qt6 SVG | libqt6svg6-dev | qt6-qtsvg-devel | qt6-svg |
| KF6 WindowSystem | libkf6windowsystem-dev | kf6-kwindowsystem-devel | extra-cmake-modules |
| KF6 Solid | libkf6solid-dev | kf6-solid-devel | extra-cmake-modules |
| Polkit Qt6 | libpolkit-qt6-1-dev | polkit-qt6-1-devel | polkit-qt6 |
| D-Bus menu | libdbusmenu-glib-dev | libdbusmenu-glib-devel | libdbusmenu-glib |
| XCB dev | libxcb-util0-dev | xcb-util-devel | xcb-util |
| X11 dev | xorg-dev | xorg-x11-server-devel | xorgproto |
| labwc | labwc | labwc (Copr) | labwc |
| sway | sway | sway | sway |
| swaybg | swaybg | swaybg | swaybg |
| swayidle | swayidle | swayidle | swayidle |
| swaylock | swaylock | swaylock | swaylock |
| wl-clipboard | wl-clipboard | wl-clipboard | wl-clipboard |
| grim | grim | grim | grim |
| slurp | slurp | slurp | slurp |
| rofi | rofi | rofi | rofi-wayland |
| dunst | dunst | dunst | dunst |
| SDDM | sddm | sddm | sddm |
| emacs | emacs-gtk | emacs | emacs |
| brightnessctl | brightnessctl | brightnessctl | brightnessctl |
| playerctl | playerctl | playerctl | playerctl |
| pavucontrol | pavucontrol | pavucontrol | pavucontrol |

## Debian/Ubuntu Notes

```bash
# Enable deb-src for build-dep
sudo sed -i 's/^Types: deb$/Types: deb deb-src/' /etc/apt/sources.list.d/ubuntu.sources

# Install build dependencies
sudo apt-get build-dep qt6-base liblxqt lxqt-panel

# Some packages may need newer versions
sudo apt install libkf6windowsystem-dev libkf6solid-dev libkf6coreaddons-dev
```

## Fedora/RHEL Notes

```bash
# Enable EPEL (RHEL/CentOS/Rocky/Alma only)
sudo dnf install -y epel-release

# Enable CRB (RHEL 9+ / CentOS Stream 9+)
sudo dnf config-manager --set-enabled crb

# Enable PowerTools (RHEL 8 / CentOS 8)
sudo dnf config-manager --set-enabled powertools

# Enable Copr for labwc
sudo dnf copr enable -y zhsj/labwc

# Or run the automated script
bash scripts/features/setup-fedora-repos.sh
```

## Arch Linux Notes

```bash
# All packages are in official repos
sudo pacman -S labwc swaybg swayidle swaylock wl-clipboard grim slurp \
    rofi-wayland dunst sddm qt6-base kvantum

# Or use the PKGBUILD
makepkg -si
```

## Package Manager Detection

All scripts in `scripts/features/` use `lib.sh` for automatic detection:

```bash
# Detect package manager
detect_pm()  # Returns: apt, dnf, pacman, zypper, or unknown

# Detect distro family
detect_distro()  # Returns: debian, fedora, rhel, suse, arch, void, alpine

# Install packages (auto-detects manager)
pkg_install "package-name"

# Install with fallback names
pkg_install_fallback "primary-name" "fallback1" "fallback2"
```

## Custom Repository Setup

### Adding a Copr Repo (Fedora)

```bash
# Using lib.sh helper
enable_copr "user/repo-name"

# Manual
sudo dnf copr enable -y user/repo-name
```

### Adding a PPA (Ubuntu)

```bash
sudo add-apt-repository ppa:user/ppa-name
sudo apt update
```
