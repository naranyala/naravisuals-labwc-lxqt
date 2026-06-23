# Architecture

Deep dive into the system architecture, data flow, and dependency relationships.

## Abstraction Layers

The codebase is organized into five distinct layers. Each layer has a single responsibility and does not depend on other layers at the same level.

```
Layer 1: apps/         Qt6 GUIs (Control Center, SDDM GUI, NTFS GUI)
Layer 2: cmd/          C++ deployment tools (dotfiles installer)
Layer 3: configs/      Static configuration files and dotfiles
Layer 4: scripts/      Bash automation (build, features, orchestration)
Layer 5: assets/       Wallpapers, design references
```

### Layer 1: apps/ — Native Management Utilities

Qt6 C++17 applications compiled with CMake + Ninja. Each app is a standalone binary that dispatches to scripts via `QProcess` or directly manipulates system files.

| App | Purpose | Qt6 Modules |
|-----|---------|-------------|
| control-center | 8-page settings hub | Core, Gui, Widgets |
| sddm-gui | SDDM theme selector | Core, Gui, Widgets |
| ntfs-gui | Partition mount/unmount with pkexec | Core, Gui, Widgets |

**Build:**
```bash
cd apps/<app> && mkdir -p build && cd build
cmake -GNinja .. && ninja
```

### Layer 2: cmd/ — Deployment Tooling

Native C++ binary for deploying dotfiles. Single-file compilation, no build system required.

| Tool | Purpose | Dependencies |
|------|---------|--------------|
| dotfiles-manager | Copies 33 config files to ~/.config/ | C++17 std::filesystem |

**Build:**
```bash
cd cmd/dotfiles-manager
c++ -std=c++17 build.cpp -o lxqt-dotfiles
```

### Layer 3: configs/ — Core Configurations

Static files organized by component. The dotfiles installer copies these to `~/.config/`. System-wide files (SDDM, wayland session) require sudo.

```
configs/
  dotfiles/           User configs (33 files)
    labwc/            Compositor: rc.xml, autostart, environment, themerc, menu.xml, shutdown
    lxqt/             Panel, session, shortcuts, QSS theme, panel-stock
    gtk-3.0/          GTK3 settings
    gtk-4.0/          GTK4 settings
    rofi/             Application launcher theme
    wob/              OSD overlay config
    cliphist/         Clipboard history config
    dunst/            Notification daemon config
    swaylock/         Screen locker config
    kanshi/           Output auto-configuration
    qt6ct/            Qt6 configuration tool
    pcmanfm-qt/       File manager settings
    qterminal.org/    Terminal settings
    sddm/             SDDM session config
    wayland-sessions/ Wayland .desktop file
    xdg-desktop-portal-wlr/  Screen sharing portal
    emacs/            Emacs init.el
    fonts/            Font files
    wallpapers/       Default wallpapers
  compositors/        Multi-compositor profiles
    hyprland/         Hyprland config
    sway/             Sway config
    wayfire/          Wayfire config
```

### Layer 4: scripts/ — Automation and Administration

Bash scripts organized by purpose. All feature scripts source `lib.sh` for shared utilities.

```
scripts/
  lib.sh              Shared library (colors, downloads, pkg helpers, error handling)
  install-all.sh      Orchestrator (20 modules)
  reset-panel.sh      Panel reset tool
  switch-lxqt-compositor.sh  Compositor switcher
  build/
    build_latest_lxqt_desktop.sh   Full LXQt from source
    manual_build_lxqt_desktop.sh   Step-by-step build
    install.sh                     Bootstrap installer
  features/
    install-clipboard.sh    wl-clipboard + cliphist
    install-osd.sh          wob OSD setup
    install-launcher.sh     rofi-wayland setup
    setup-portals.sh        XDG portal setup
    setup-fedora-repos.sh   EPEL/CRB/Copr enablement
    install-kvantum.sh      Kvantum + Qt6CT
    wallust-setup.sh        Dynamic color extraction
    install-compositor.sh   Compositor installer
  themes.sh, icons.sh, cursors.sh, fonts.sh, wallpapers.sh
  labwc-themes.sh, lxqt-themes.sh
  conky.sh, neofetch.sh, emacs.sh
  fix-gtk-window-controls.sh, enable-natural-scrollong.sh, enable-touchpad-tap.sh
  set_default_wallpaper.sh, update_sddm_wallpaper.sh, update_lxqt_panel.sh
```

### Layer 5: assets/ — Media and Resources

Static assets used by configuration files.

| Asset | Size | Purpose |
|-------|------|---------|
| wallpaper.jpg | 465 KB | Default desktop wallpaper |
| inspiration/ | 127 MB | Design reference images (excluded from archive) |

## Data Flow

```
User runs install.sh
  |
  +-- check_prereqs()          Validates network, disk, sudo, distro
  |
  +-- Build dotfiles installer (c++ build.cpp)
  |
  +-- dotfiles-manager install  Copies 33 configs to ~/.config/
  |
  +-- install-all.sh            Runs feature modules
  |     |
  |     +-- install-clipboard.sh
  |     +-- install-osd.sh
  |     +-- install-launcher.sh
  |     +-- setup-portals.sh
  |     +-- themes.sh, icons.sh, cursors.sh, fonts.sh
  |     +-- ...
  |
  +-- Build Control Center (cmake + ninja)
  |
  +-- Summary with log file path
```

## Control Center Dispatch Flow

```
User clicks button in Control Center
  |
  +-- QProcess::startDetached("bash", {script_path})
  |
  +-- Script runs (sources lib.sh)
  |     |
  |     +-- detect_pm()     Identifies package manager
  |     +-- detect_distro() Identifies distro family
  |     +-- pkg_install()   Installs via correct package manager
  |
  +-- Script modifies config files or installs packages
  |
  +-- User sees result (QMessageBox or status bar)
```

## Dotfiles Installer Flow

```
cmd/dotfiles-manager/build.cpp
  |
  +-- Reads manifest (33 src/dst pairs)
  |
  +-- For each entry:
  |     +-- Resolve source path (relative to CWD)
  |     +-- Resolve destination ($HOME/.config/...)
  |     +-- Create parent directories
  |     +-- Copy file (overwrite existing)
  |     +-- Set permissions (0755 for autostart/shutdown)
  |
  +-- Print summary (installed/skipped/failed)
  +-- Print manual steps (SDDM, wayland session)
```

## Error Handling Architecture

```
lib.sh provides:
  |
  +-- log_* functions        Write to stdout + log file
  +-- check_prereqs()        Validates entire environment
  +-- check_network()        Tests connectivity
  +-- check_disk()           Verifies disk space
  +-- confirm()              Interactive Y/n prompt
  +-- retry()                Exponential backoff retry
  +-- die()                  Fatal error with cleanup
  +-- on_cleanup()           Register cleanup handlers
  +-- trap ERR               Automatic error logging
```
