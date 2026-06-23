# Naravisuals System Configuration Framework

## Overview

The Naravisuals System Configuration Framework provides a comprehensive deployment and management layer for executing the LXQt 2.x desktop environment atop the Labwc Wayland compositor. By integrating native C++ tooling, Qt6-based graphical management utilities, curated configuration specifications, and cross-distribution automation scripts, this framework delivers a consistent, production-ready Linux desktop environment.

---

## Table of Contents

1. [Architecture](#architecture)
2. [Dependency Mapping](#dependency-mapping)
3. [Repository Structure](#repository-structure)
4. [Installation Procedures](#installation-procedures)
5. [Configuration Manifest](#configuration-manifest)
6. [Graphical Control Center](#graphical-control-center)
7. [Minimal Applications (Qt ↔ GTK)](#minimal-applications-qt--gtk)
8. [Scripting Reference](#scripting-reference)
9. [Keybindings Reference](#keybindings-reference)
10. [Environment Variables](#environment-variables)
11. [Theming Engine](#theming-engine)
12. [Distribution Support](#distribution-support)
13. [Troubleshooting Guide](#troubleshooting-guide)

---

## Architecture

The framework is strictly organized into five functional layers, maintaining separation of concerns to prevent direct coupling.

- **Layer 1: Graphical Interfaces (`apps/`)**: Native Qt6 applications, including the Control Center and Display Manager interfaces.
- **Layer 2: Core Tooling (`cmd/`)**: High-performance C++ deployment binaries responsible for configuration distribution.
- **Layer 3: Specifications (`configs/`)**: Static dotfiles and hierarchical configuration trees.
- **Layer 4: Automation (`scripts/`)**: Shell automation for build processes, feature deployments, and orchestration.
- **Layer 5: Assets (`assets/`)**: Essential visual assets, including wallpapers and design references.

### Data Flow Execution

The Control Center functions as an administrative frontend that dispatches instructions to underlying scripts via the `QProcess` class. The automation scripts execute system-wide changes, while the dotfiles installer operates as an independent binary to rapidly deploy configurations without requiring runtime dependencies.

---

## Dependency Mapping

### Core Desktop Stack (Mandatory)
- **LXQt 2.x (Qt6)**: The foundational desktop environment.
- **lxqt-panel**: Provides the primary interface including taskbar, tray, volume, and clock plugins.
- **lxqt-session**: Manages session initialization and autostart routines.
- **lxqt-config**: Exposes core system settings.
- **lxqt-runner**: Facilitates application launching.
- **lxqt-policykit**: Manages privilege escalation requests.
- **lxqt-globalkeys**: Daemons for system-wide shortcuts.

### Wayland Compositor (Mandatory)
- **Labwc (>= 0.7.1)**: The primary Wayland compositor, built atop `wlroots`. Reads input variables from `rc.xml` and `environment`.

### Auxiliary Tooling (Recommended)
- **swaybg**: Wallpaper rendering backend.
- **swayidle & swaylock**: Idle detection and session locking.
- **grim, slurp, & grimshot**: Screenshot capture pipeline.
- **wl-clipboard & cliphist**: Clipboard history management.
- **wob**: Wayland Overlay Bar for visual feedback regarding volume and brightness.
- **rofi-wayland**: Advanced application launcher.
- **dunst**: Lightweight notification daemon.
- **xdg-desktop-portal-wlr**: Screen sharing and sandboxing portal framework.

---

## Repository Structure

```text
naravisuals-labwc-lxqt/
├── apps/               # Native Qt6 management applications
├── cmd/                # C++ deployment binaries
├── configs/            # Application configuration specifications
├── scripts/            # Cross-distribution deployment scripts
├── assets/             # Visual resources
├── lxqt/               # Full LXQt source tree
├── src/                # Forked components requiring source builds
└── thirdparty/         # External dependencies and archives
```

---

## Installation Procedures

### Prerequisites

Ensure the target system meets the following minimum requirements:
- **Compiler**: GCC 9+ or Clang 10+ (C++17 specification required)
- **Build Systems**: CMake 3.16+ and Ninja 1.10+
- **Frameworks**: Qt6 6.5+
- **Compositor**: Labwc 0.7.1+
- **Desktop**: LXQt 2.1+

Supported operating systems include Debian 12+, Ubuntu 22.04+, Fedora 38+, Rocky/AlmaLinux 9+, Arch Linux, and openSUSE Tumbleweed.

### Phase 1: Source Compilation (Optional)

For distributions lacking updated LXQt or Qt6 packages, execute the source build script:

```bash
sudo ./scripts/build/build_latest_lxqt_desktop.sh
```

For RHEL/Fedora derivatives, additional repositories must be enabled prior to compilation:

```bash
bash scripts/features/setup-fedora-repos.sh
```

### Phase 2: Configuration Deployment

Compile and execute the native configuration installer:

```bash
cd cmd/dotfiles-manager
c++ -std=c++17 build.cpp -o lxqt-dotfiles
./lxqt-dotfiles install
```

System-wide display manager configurations must be applied with elevated privileges:

```bash
sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/
sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/
```

### Phase 3: Feature Orchestration

Deploy optional system components using the orchestration script:

```bash
bash scripts/install-all.sh --full
```

---

## Graphical Control Center

The Control Center provides administrative access to the system configuration. 

### Compilation

```bash
cd apps/control-center
mkdir -p build && cd build
cmake -GNinja ..
ninja
./nv-control-center
```

### Modules
- **Appearance**: Adjust display manager themes, cursors, and system fonts.
- **Compositors**: Dynamically switch between Labwc, Hyprland, Sway, and Wayfire.
- **Theming**: Manage Qt6 styling via Kvantum and extract dynamic color palettes via Wallust.
- **System**: Configure XDG portals and rebuild panel components from source.

---

## Minimal Applications (Qt ↔ GTK)

The framework ships a minimal apps installer (`scripts/install-minimal-apps.sh`) that provisions lightweight Qt-based applications alongside their GTK equivalents. Packages are resolved with distro-aware fallbacks — the first available package in each row is installed.

### Usage

```bash
bash scripts/install-minimal-apps.sh --list       # Show mapping table
bash scripts/install-minimal-apps.sh --qt-only    # Qt apps only
bash scripts/install-minimal-apps.sh --gtk-only   # GTK equivalents only
bash scripts/install-minimal-apps.sh --both       # Install both
bash scripts/install-minimal-apps.sh --select     # Pick categories
bash scripts/install-minimal-apps.sh --dry-run    # Preview only
```

### Application Mapping

| Category | Description | Qt Package(s) | GTK Equivalent(s) |
|----------|-------------|---------------|-------------------|
| Audio | Volume Control | `pavucontrol-qt` | `pavucontrol` |
| Display | Monitor Settings | `lxqt-config-monitor` / `lxqt-config` | `arandr` / `wdisplays` / `gnome-control-center` |
| Network | WiFi Manager | `qconnman-ui` / `connman-qt` | `nm-connection-editor` / `network-manager-gnome` / `connman-gtk` |
| Bluetooth | Bluetooth Manager | `bluedevil` / `qbluez` | `blueman` / `gnome-bluetooth` |
| Power | Power Management | `lxqt-powermanagement` / `lxqt-config` | `xfce4-power-manager` / `gnome-power-manager` / `mate-power-manager` |
| Input | Keyboard / Mouse / Touchpad | `lxqt-config-input` / `lxqt-config` | `xinput` / `xfce4-settings` |
| Disk | Partition Info | `partitionmanager` / `kde-partitionmanager` | `gnome-disk-utility` / `gparted` / `baobab` |
| Theme | Appearance | `qt6ct` / `kvantum-qt6` | `lxappearance` / `nwg-look` |
| File Manager | File Browser | `pcmanfm-qt` | `pcmanfm` / `thunar` / `nautilus` / `nemo` / `caja` |
| Terminal | Terminal Emulator | `qterminal` | `xfce4-terminal` / `mate-terminal` / `lxterminal` / `alacritty` / `foot` |
| Editor | Text Editor | `featherpad` / `kate` | `mousepad` / `xed` / `pluma` / `gedit` / `l3afpad` |
| Image Viewer | Image Viewer | `lximage-qt` / `nomacs` | `eog` / `eom` / `ristretto` / `feh` / `imv` |
| PDF | Document Viewer | `qpdfview` / `okular` | `evince` / `atril` / `zathura` |
| Archive | Archive Manager | `lxqt-archiver` / `ark` | `file-roller` / `engrampa` / `xarchiver` |
| Screenshot | Screenshot Tool | `spectacle` / `lxqt-screenshot` | `gnome-screenshot` / `xfce4-screenshooter` / `flameshot` |
| System Monitor | Task Monitor | `qps` / `lxqt-admin` | `gnome-system-monitor` / `mate-system-monitor` / `xfce4-taskmanager` / `htop` |
| Notification | Notification Daemon | `lxqt-notificationd` | `dunst` / `mako` / `swaync` |
| Wallpaper | Wallpaper Setter | `swaybg` | `nitrogen` / `swaybg` / `feh` |
| Clipboard | Clipboard Manager | `qlipper` / `copyq` | `parcellite` / `clipit` / `gpaste` |
| Launcher | Application Launcher | `lxqt-runner` | `rofi-wayland` / `rofi` / `dmenu` / `wofi` / `fuzzel` / `bemenu` |
| Lock Screen | Lock Screen | `light-locker` | `swaylock` / `swaylock-effects` / `gtklock` / `i3lock` |
| Polkit | Auth Agent | `lxqt-policykit` | `polkit-gnome` / `mate-polkit` / `xfce-polkit` |
| Session | Session Manager | `lxqt-session` | `xfce4-session` / `mate-session-manager` |

### Category Groups (for `--select` mode)

| Group | Apps Included |
|-------|---------------|
| Essential Desktop | File manager, terminal, text editor, polkit, session |
| Multimedia | Audio, image viewer, screenshot |
| System Utils | Disk, system monitor, power, input, display |
| Connectivity | Network, bluetooth |
| Appearance | Theme, wallpaper, notification |
| Productivity | PDF viewer, archive, clipboard, launcher, lock screen |

---

## Keybindings Reference

System-wide keybindings are managed through `configs/dotfiles/labwc/rc.xml`.

| Binding | Assigned Action |
|---------|-----------------|
| Super+Enter | Launch default terminal |
| Super+E | Launch file manager |
| Super+Q | Terminate active window |
| Super+M | Toggle maximization state |
| Super+F | Toggle fullscreen state |
| Super+V | Open audio mixer panel |
| Super+L | Terminate session |
| Alt+F2 | Invoke application launcher |
| Print Screen | Capture full display |
| Shift+Print Screen | Capture selected region |

---

## Environment Variables

Environment initialization occurs during session startup via `configs/dotfiles/labwc/environment`.

| Variable | Value | Purpose |
|----------|-------|---------|
| XDG_CURRENT_DESKTOP | LXQt:labwc:wlroots | Standard desktop identification |
| QT_QPA_PLATFORMTHEME | qt6ct | Standardizes Qt6 application themes |
| GDK_BACKEND | wayland,x11 | Defines GTK rendering preference |
| MOZ_ENABLE_WAYLAND | 1 | Enables Wayland backend for Firefox |

---

## Theming Engine

The framework employs a strictly coordinated Nord-inspired color palette applied across window decorations, widgets, and panel interfaces.

- **Qt Widgets**: Managed via Kvantum through `qt6ct.conf`.
- **GTK Widgets**: Controlled by the `settings.ini` files within `gtk-3.0` and `gtk-4.0`.
- **Panel Interfaces**: Styled using Qt Style Sheets (`lxqt-panel.qss`).

To deploy the required aesthetic packages, execute the relevant administrative scripts:

```bash
bash scripts/themes.sh --all
bash scripts/icons.sh --all
bash scripts/cursors.sh --all
```

---

## Distribution Support

The `scripts/lib.sh` library automatically detects and interfaces with the host package manager, ensuring operational parity across Debian (apt), Fedora (dnf), Arch (pacman), and openSUSE (zypper) systems.

---

## Troubleshooting Guide

### Display Anomaly: Missing Window Controls
The LXQt panel requires the `wlr-foreign-toplevel` protocol. Ensure Labwc is correctly installed and executing.

### Incorrect Application Theming
Verify that the `QT_QPA_PLATFORMTHEME` variable is correctly set to `qt6ct` within the session environment file, and that the Kvantum backend is correctly populated.

### Clipboard Inoperability
Ensure the clipboard daemon is running in the background. Verify process execution:

```bash
pgrep -a cliphist
```

### Screen Sharing Failures
Wayland screen sharing requires proper portal initialization. Run the portal configuration tool:

```bash
bash scripts/features/setup-portals.sh
```

---

## License

This software framework distributes configurations and management tools for the LXQt desktop environment. Individual software dependencies retain their original licensing structures. The configurations and automation tools provided in this repository are supplied as-is.
