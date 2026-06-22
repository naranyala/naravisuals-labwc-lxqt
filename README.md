# Naravisuals Desktop Environment Suite

A complete configuration, deployment, and management layer for running LXQt 2.x on the Labwc Wayland compositor. The suite provides native C++ tooling, Qt6 graphical management, curated dotfiles, and cross-distro automation scripts to produce a consistent, production-ready Linux desktop.

---

## Table of Contents

1. [Architecture](#architecture)
2. [Dependency Map](#dependency-map)
3. [Directory Structure](#directory-structure)
4. [Installation](#installation)
5. [Dotfiles Manifest](#dotfiles-manifest)
6. [Control Center](#control-center)
7. [Scripts Reference](#scripts-reference)
8. [Keybindings](#keybindings)
9. [Environment Variables](#environment-variables)
10. [Theming](#theming)
11. [Distro Support](#distro-support)
12. [Troubleshooting](#troubleshooting)

---

## Architecture

The repository is organized into five abstraction layers. Each layer has a distinct responsibility and avoids direct coupling to other layers.

```
Layer 1: apps/         Native Qt6 GUIs (Control Center, SDDM GUI)
Layer 2: cmd/          C++ deployment binaries (dotfiles installer)
Layer 3: configs/      Static dotfiles and configuration trees
Layer 4: scripts/      Bash automation (build, features, orchestration)
Layer 5: assets/       Wallpapers, mockups, visual references
```

**Data flow:**

```
configs/ --[cmd/dotfiles-manager]--> ~/.config/
scripts/ --[executed by user or apps/]--> system-wide changes
apps/    --[QProcess]--> scripts/
lxqt/    --[build script]--> /usr/local/
```

The Control Center (`apps/control-center/`) acts as a frontend that dispatches to scripts via `QProcess`. Scripts never call the Control Center. The dotfiles installer (`cmd/dotfiles-manager/`) is a standalone binary that copies configs without runtime dependencies on Qt6 or Bash.

---

## Dependency Map

This section documents every external dependency, what requires it, and how the dependencies relate to each other.

### Core Stack (required)

```
LXQt 2.x (Qt6)
  |
  +-- lxqt-panel         Panel with plugins (taskbar, tray, volume, clock)
  |     +-- plugin-tray   Requires statusnotifier (SNI) protocol
  |     +-- plugin-volume Requires PulseAudio or PipeWire
  |     +-- plugin-clock  Standalone
  |
  +-- lxqt-session        Session manager, autostart handler
  +-- lxqt-config         Settings GUI (appearance, input, monitor)
  +-- lxqt-runner         Application launcher (Alt+F2)
  +-- lxqt-policykit      Privilege escalation agent
  +-- lxqt-notificationd  Notification daemon (alternative: dunst)
  +-- lxqt-globalkeys     Global keyboard shortcut daemon
  +-- lxqt-powermanagement Lid close, suspend, hibernate
  +-- pcmanfm-qt          File manager and desktop icon handler
  +-- qterminal           Terminal emulator
  +-- qtermwidget         Terminal widget (used by qterminal)
```

### Wayland Compositor (required)

```
Labwc (>= 0.7.1)
  |
  +-- wlroots             Underlying Wayland library
  +-- libxml2             Parses rc.xml, menu.xml
  +-- libinput            Input device handling
  +-- wlr-protocols       Wayland protocol extensions
```

Labwc reads `~/.config/labwc/rc.xml` for keybindings and `~/.config/labwc/environment` for session variables. It supports Openbox window decoration themes via `~/.config/labwc/themerc-override`.

### Wayland Companion Tools (required)

| Tool | Purpose | Required By |
|------|---------|-------------|
| swaybg | Wallpaper rendering | autostart (wallpaper display) |
| swayidle | Screen idle management | autostart (screen blank, lock) |
| swaylock | Screen locker | swayidle (lock command) |
| grim | Screenshot capture | rc.xml (Print key) |
| slurp | Area selection for screenshots | grim (piped selection) |
| grimshot | Screenshot wrapper script | rc.xml (Print key) |

### Clipboard Stack (optional, recommended)

```
wl-clipboard             Base clipboard utilities (wl-copy, wl-paste)
  |
  +-- cliphist           Clipboard history manager (stores entries)
       |
       +-- wofi/rofi     Launcher reads from cliphist for history UI
```

The autostart script launches `wl-paste --watch cliphist store` in the background. The user invokes the history via a keybinding that pipes `cliphist list` into a launcher.

### OSD Stack (optional, recommended)

```
wob (Wayland Overlay Bar)
  |
  +-- /tmp/wob_fifo      Named pipe for progress data
  +-- pactl              Writes volume percentage to FIFO
  +-- brightnessctl      Writes brightness percentage to FIFO
```

Volume and brightness keybindings in `rc.xml` pipe the current level through `wob` for visual feedback.

### Application Launcher (optional, recommended)

```
rofi-wayland
  |
  +-- reads ~/.config/rofi/config.rasi (Nord theme included)
  +-- replaces lxqt-runner as Alt+F2 handler
  +-- supports drun, run, window modes
```

### Notification Stack

```
dunst                    Preferred (lightweight, Wayland layer-shell)
  |
  +-- reads ~/.config/dunst/dunstrc

lxqt-notificationd       Fallback (built into LXQt)
```

The autostart prefers dunst if installed, otherwise falls back to lxqt-notificationd.

### Portal Stack (recommended for Flatpak/screen sharing)

```
xdg-desktop-portal        Base portal daemon
  |
  +-- xdg-desktop-portal-wlr   wlroots backend (screen capture via PipeWire)
  +-- xdg-desktop-portal-gtk   GTK file chooser backend
  +-- xdg-desktop-portal-lxqt  LXQt native file chooser (if available)
```

Portals are required for Flatpak sandboxed applications and for screen sharing in Wayland. The autostart launches the portal daemons.

### Display Manager

```
SDDM (>= 0.20)
  |
  +-- Qt6-based, supports Wayland sessions
  +-- reads /etc/sddm.conf.d/*.conf
  +-- themes in /usr/share/sddm/themes/
```

### Input Management

```
libinput                  Touchpad/mouse configuration
  |
  +-- configured via rc.xml <libinput> section
  +-- supports: natural scroll, tap-to-click, disable-while-typing
  +-- pointer speed, accel profile, scroll method
```

### Audio Stack

```
PipeWire (preferred) or PulseAudio
  |
  +-- pactl               CLI volume control (used in rc.xml keybindings)
  +-- pavucontrol         GUI mixer (Super+V)
  +-- playerctl           Media player control (XF86 keys)
```

### Screenshot Stack

```
grim + slurp              Wayland-native screenshot pipeline
  |
  +-- grimshot            Wrapper: grimshot copy screen/area/window
  +-- bound to Print, Shift+Print, Ctrl+Print in rc.xml
```

### GTK Integration

```
GTK 3.0 / GTK 4.0
  |
  +-- ~/.config/gtk-3.0/settings.ini   Theme, icons, cursor
  +-- ~/.config/gtk-4.0/settings.ini   Theme, icons, cursor
  +-- gsettings syncs cursor from LXQt session.conf
```

The autostart script reads cursor settings from `session.conf` and applies them to GTK via `gsettings`.

### Power Management

```
swayidle                  Idle detection daemon
  |
  +-- wlopm               Wayland power management (screen off/on)
  +-- swaylock             Lock on idle timeout
  +-- before-sleep hook    Lock before suspend
```

### Dynamic Output Management

```
kanshi                    Auto-configures displays on hotplug
  |
  +-- reads ~/.config/kanshi/config
  +-- applies profiles based on connected outputs
```

---

## Directory Structure

```
naravisuals-labwc-lxqt/
|
+-- apps/
|   +-- control-center/        Qt6 settings GUI (CMake + Ninja)
|   |   +-- CMakeLists.txt
|   |   +-- main.cpp
|   +-- sddm-gui/              Qt6 SDDM theme selector (CMake + Ninja)
|       +-- CMakeLists.txt
|       +-- main.cpp
|
+-- cmd/
|   +-- dotfiles-manager/      C++17 native dotfiles installer
|       +-- build.cpp          Single-file compiler (c++ -std=c++17)
|       +-- lxqt-dotfiles      Pre-built binary
|
+-- configs/
|   +-- dotfiles/
|   |   +-- labwc/             Compositor configs (rc.xml, autostart, environment, themerc)
|   |   +-- lxqt/              Panel, session, runner, shortcuts, panel-stock.conf
|   |   +-- gtk-3.0/           GTK3 settings (theme, icons, cursor)
|   |   +-- gtk-4.0/           GTK4 settings
|   |   +-- qt6ct/             Qt6 Configuration Tool
|   |   +-- dunst/             Notification daemon config
|   |   +-- swaylock/          Screen locker config
|   |   +-- kanshi/            Output auto-configuration
|   |   +-- rofi/              Application launcher (Nord theme)
|   |   +-- wob/               OSD overlay bar config
|   |   +-- cliphist/          Clipboard history config
|   |   +-- xdg-desktop-portal-wlr/  Screen sharing portal
|   |   +-- pcmanfm-qt/        File manager settings
|   |   +-- qterminal.org/     Terminal settings
|   |   +-- sddm/              SDDM session definition
|   |   +-- wayland-sessions/  Wayland session .desktop file
|   |   +-- emacs/             Emacs init.el
|   |   +-- fonts/             Font files
|   |   +-- wallpapers/        Default wallpapers
|   +-- lxqt-dotfiles          Symlink to dotfiles-manager binary
|
+-- configs/compositors/
|   +-- hyprland/              Hyprland config (animations, blur, tiling)
|   +-- sway/                  Sway config (i3-compatible tiling)
|   +-- wayfire/               Wayfire config (3D compositor)
|
+-- scripts/
|   +-- lib.sh                 Shared library (colors, download, pkg helpers)
|   +-- install-all.sh         Orchestrator (15 modules)
|   +-- build/
|   |   +-- build_latest_lxqt_desktop.sh   Full LXQt from source
|   |   +-- manual_build_lxqt_desktop.sh   Manual step-by-step build
|   |   +-- install.sh                     Bootstrap installer
|   +-- features/
|   |   +-- install-clipboard.sh    wl-clipboard + cliphist
|   |   +-- install-osd.sh          wob OSD setup
|   |   +-- install-launcher.sh     rofi-wayland setup
|   |   +-- setup-portals.sh        XDG portal setup
|   |   +-- setup-fedora-repos.sh   EPEL/CRB/Copr enablement
|   +-- reset-panel.sh             Panel reset to stock config
|   +-- switch-lxqt-compositor.sh  Compositor switcher
|   +-- themes.sh                  GTK/Qt theme downloader
|   +-- icons.sh                   Icon pack installer
|   +-- cursors.sh                 Cursor theme installer
|   +-- fonts.sh                   Font installer
|   +-- wallpapers.sh              Wallpaper pack downloader
|   +-- labwc-themes.sh            Window decoration installer
|   +-- lxqt-themes.sh             LXQt widget theme installer
|   +-- conky.sh                   Conky setup
|   +-- neofetch.sh                System fetch tool
|   +-- emacs.sh                   Emacs installer + config
|   +-- fix-gtk-window-controls.sh GTK button layout fix
|   +-- enable-natural-scrollong.sh  Touchpad natural scroll
|   +-- enable-touchpad-tap.sh       Touchpad tap-to-click
|   +-- set_default_wallpaper.sh     Wallpaper setter
|   +-- update_sddm_wallpaper.sh     SDDM background updater
|   +-- update_lxqt_panel.sh         Panel rebuild from source
|
+-- assets/
|   +-- wallpaper.jpg            Default wallpaper
|   +-- inspiration/             Design reference images
|
+-- lxqt/                       Full LXQt source tree (git submodules)
+-- src/
|   +-- lxqt-panel/             Forked LXQt panel with custom plugins
+-- thirdparty/
    +-- xfce4-ddc-brightness-slider/  DDC brightness control
    +-- lxqt-2.4.0.zip               LXQt release archive
```

---

## Installation

### Prerequisites

| Requirement | Minimum Version |
|-------------|-----------------|
| C++ compiler | GCC 9+ or Clang 10+ (C++17 support) |
| CMake | 3.16+ |
| Ninja | 1.10+ |
| Qt6 | 6.5+ |
| Labwc | 0.7.1+ |
| LXQt | 2.1+ |

Supported distributions: Debian 12+, Ubuntu 22.04+, Fedora 38+, Rocky/Alma 9+, Arch Linux, openSUSE Tumbleweed.

### Step 1: Build LXQt from Source (optional)

If your distro ships an outdated LXQt or lacks Qt6 packages, build from source:

```bash
sudo ./scripts/build/build_latest_lxqt_desktop.sh
```

This script:
1. Detects your package manager (apt/dnf) and installs build dependencies
2. Downloads and compiles a lightweight Qt6 (skipping webengine, 3d, multimedia)
3. Clones the LXQt repository with all submodules
4. Builds and installs all LXQt components via the upstream build script

For Fedora/RHEL, enable additional repos first:

```bash
bash scripts/features/setup-fedora-repos.sh
```

### Step 2: Install Dotfiles

Compile the native C++ installer and deploy configurations:

```bash
cd cmd/dotfiles-manager
c++ -std=c++17 build.cpp -o lxqt-dotfiles
./lxqt-dotfiles install
```

The installer copies 28 configuration files from `configs/dotfiles/` to `~/.config/`. It sets executable permissions on `autostart` and `shutdown` scripts. System-wide files (SDDM config, session .desktop) must be copied manually:

```bash
sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/
sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/
```

### Step 3: Install Feature Modules

Run the orchestrator to install optional components:

```bash
bash scripts/install-all.sh --select      # Interactive menu
bash scripts/install-all.sh --all         # Install everything
bash scripts/install-all.sh themes icons  # Specific modules only
```

Available modules: Fedora repos, clipboard, OSD, launcher, portals, themes, icons, cursors, fonts, wallpapers, labwc themes, LXQt themes, neofetch, conky, emacs.

### Step 4: Build the Control Center (optional)

```bash
cd apps/control-center
mkdir -p build && cd build
cmake -GNinja ..
ninja
./naravisuals-control-center
```

Requires Qt6::Core, Qt6::Gui, Qt6::Widgets.

### Step 5: Reset Panel (if needed)

If the panel layout needs to be restored to the stock configuration:

```bash
bash scripts/reset-panel.sh              # Interactive
bash scripts/reset-panel.sh --force      # No prompt
bash scripts/reset-panel.sh --restore    # Undo from backup
```

---

## Packaging

### One-Command Installer

```bash
bash install.sh --full       # Install everything
bash install.sh --minimal    # Core configs only
bash install.sh --select     # Choose modules interactively
bash install.sh --dry-run    # Preview without changes
```

### Arch Linux (AUR)

A `PKGBUILD` is provided for building an Arch package:

```bash
makepkg -si
```

The package installs:
- Binaries to `/usr/bin/` (naravisuals-dotfiles, naravisuals-control-center, naravisuals-sddm-gui, naravisuals-reset-panel)
- Configs to `/etc/xdg/naravisuals/` (system-wide defaults)
- Scripts to `/usr/share/naravisuals/scripts/`
- Compositor profiles to `/etc/xdg/naravisuals/compositors/`

After package install, run the dotfiles installer to deploy to user home:

```bash
naravisuals-dotfiles install
```

### Manual Build

```bash
# Build dotfiles installer
cd cmd/dotfiles-manager
c++ -std=c++17 build.cpp -o lxqt-dotfiles
./lxqt-dotfiles install

# Build Control Center
cd apps/control-center
mkdir -p build && cd build
cmake -GNinja .. && ninja
./naravisuals-control-center
```

---

## Dotfiles Manifest

The C++ installer manages these 33 files:

| Source (configs/dotfiles/) | Destination (~/.config/) | Purpose |
|---|---|---|
| lxqt/session.conf | lxqt/session.conf | Session startup, cursor, theme |
| lxqt/panel.conf | lxqt/panel.conf | Panel layout and plugins |
| lxqt/lxqt.conf | lxqt/lxqt.conf | Core LXQt settings |
| lxqt/lxqt-config.conf | lxqt/lxqt-config.conf | Config center preferences |
| lxqt/lxqt-powermanagement.conf | lxqt/lxqt-powermanagement.conf | Power management |
| lxqt/lxqt-runner.conf | lxqt/lxqt-runner.conf | Application runner |
| lxqt/lxqt-notificationd.conf | lxqt/lxqt-notificationd.conf | Notification daemon |
| lxqt/globalkeyshortcuts.conf | lxqt/globalkeyshortcuts.conf | Global shortcuts |
| labwc/rc.xml | labwc/rc.xml | Keybindings, window rules, libinput |
| labwc/menu.xml | labwc/menu.xml | Root and client menus |
| labwc/autostart | labwc/autostart | Session startup daemons |
| labwc/environment | labwc/environment | Session environment variables |
| labwc/themerc | labwc/themerc-override | Window decoration theme |
| labwc/shutdown | labwc/shutdown | Session shutdown hook |
| gtk-3.0/settings.ini | gtk-3.0/settings.ini | GTK3 theme, icons, cursor |
| gtk-4.0/settings.ini | gtk-4.0/settings.ini | GTK4 theme, icons, cursor |
| qt6ct/qt6ct.conf | qt6ct/qt6ct.conf | Qt6 appearance configuration |
| pcmanfm-qt/lxqt/settings.conf | pcmanfm-qt/lxqt/settings.conf | File manager settings |
| qterminal.org/qterminal.ini | qterminal.org/qterminal.ini | Terminal emulator settings |
| user-dirs.dirs | user-dirs.dirs | XDG user directories |
| kanshi/config | kanshi/config | Display auto-configuration |
| swaylock/config | swaylock/config | Screen locker appearance |
| dunst/dunstrc | dunst/dunstrc | Notification daemon config |
| emacs/init.el | emacs/init.el | Emacs configuration |
| cliphist/config | cliphist/config | Clipboard history config |
| wob/config | wob/config | OSD overlay bar config |
| rofi/config.rasi | rofi/config.rasi | Application launcher theme |
| xdg-desktop-portal-wlr/config | xdg-desktop-portal-wlr/config | Screen sharing portal |
| lxqt/lxqt-panel.qss | lxqt/lxqt-panel.qss | Panel QSS theme (Nord) |
| lxqt/panel-stock.conf | lxqt/panel-stock.conf | Stock panel reference |
| compositors/hyprland/hyprland.conf | hypr/hyprland.conf | Hyprland compositor config |
| compositors/sway/config | sway/config | Sway compositor config |
| compositors/wayfire/wayfire.ini | wayfire/wayfire.ini | Wayfire compositor config |

---

## Control Center

The Control Center is a Qt6 GUI application with eight pages:

| Page | Features |
|------|----------|
| Appearance | SDDM theme selector, GTK window controls fix, system themes, icon packs, cursor themes, custom fonts |
| Desktop and WM | Compositor switcher, LXQt theme installer, Labwc theme installer, panel reset to stock |
| Wallpapers | SDDM wallpaper updater, default desktop wallpaper setter, wallpaper pack downloader |
| Addons and Apps | Clipboard manager, OSD overlay, app launcher, portal setup, Conky, Neofetch, Emacs, panel rebuild |
| Input Devices | Natural scrolling enable, touchpad tap-to-click enable |
| Compositors | Install Hyprland, Sway, or Wayfire; switch active compositor |
| Theming | Install Kvantum + Qt6CT, install wallust for dynamic theming, apply colors from wallpaper, reset panel |
| System | XDG portal setup, Fedora/RHEL repo setup, panel rebuild from source |

Build and run:

```bash
cd apps/control-center && mkdir -p build && cd build
cmake -GNinja .. && ninja
./naravisuals-control-center
```

The SDDM GUI is a separate tool for selecting and applying SDDM login manager themes:

```bash
cd apps/sddm-gui && mkdir -p build && cd build
cmake -GNinja .. && ninja
./sddm-gui
```

---

## Scripts Reference

### Build Scripts

| Script | Purpose | Privileges |
|--------|---------|------------|
| `scripts/build/build_latest_lxqt_desktop.sh` | Build Qt6 + LXQt from source | sudo |
| `scripts/build/manual_build_lxqt_desktop.sh` | Step-by-step manual build | sudo |
| `scripts/build/install.sh` | Bootstrap installer | sudo |

### Feature Scripts

| Script | Purpose | Dependencies |
|--------|---------|--------------|
| `scripts/features/install-clipboard.sh` | Install wl-clipboard + cliphist | wl-clipboard, cliphist |
| `scripts/features/install-osd.sh` | Install wob, patch rc.xml for OSD | wob |
| `scripts/features/install-launcher.sh` | Install rofi, replace lxqt-runner keybinding | rofi-wayland |
| `scripts/features/setup-portals.sh` | Install and configure XDG portals | xdg-desktop-portal-wlr |
| `scripts/features/setup-fedora-repos.sh` | Enable EPEL, CRB, Copr on Fedora/RHEL | dnf |
| `scripts/features/install-kvantum.sh` | Install Kvantum + Qt6CT for SVG theming | kvantum, qt6ct |
| `scripts/features/wallust-setup.sh` | Install wallust for wallpaper color extraction | wallust (cargo) |
| `scripts/features/install-compositor.sh` | Install and configure a Wayland compositor | hyprland/sway/wayfire |

### Management Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| `scripts/install-all.sh` | Module orchestrator | `bash install-all.sh --all` |
| `scripts/reset-panel.sh` | Reset panel to stock config | `bash reset-panel.sh` |
| `scripts/switch-lxqt-compositor.sh` | Switch active compositor | `bash switch-lxqt-compositor.sh` |
| `scripts/themes.sh` | Download GTK/Qt themes | `bash themes.sh --all` |
| `scripts/icons.sh` | Download icon packs | `bash icons.sh --all` |
| `scripts/cursors.sh` | Download cursor themes | `bash cursors.sh --all` |
| `scripts/fonts.sh` | Install Nerd Fonts | `bash fonts.sh --all` |
| `scripts/wallpapers.sh` | Download wallpaper packs | `bash wallpapers.sh --all` |
| `scripts/labwc-themes.sh` | Install Labwc window decorations | `bash labwc-themes.sh --all` |
| `scripts/lxqt-themes.sh` | Install LXQt widget themes | `bash lxqt-themes.sh --all` |
| `scripts/conky.sh` | Install and configure Conky | `bash conky.sh` |
| `scripts/neofetch.sh` | Install system fetch tool | `bash neofetch.sh` |
| `scripts/emacs.sh` | Install Emacs + config | `bash emacs.sh --install` |
| `scripts/fix-gtk-window-controls.sh` | Fix GTK window button layout | `bash fix-gtk-window-controls.sh` |
| `scripts/enable-natural-scrollong.sh` | Enable touchpad natural scroll | `bash enable-natural-scrollong.sh` |
| `scripts/enable-touchpad-tap.sh` | Enable touchpad tap-to-click | `bash enable-touchpad-tap.sh` |
| `scripts/set_default_wallpaper.sh` | Set default desktop wallpaper | `bash set_default_wallpaper.sh` |
| `scripts/update_sddm_wallpaper.sh` | Update SDDM login background | `bash update_sddm_wallpaper.sh` |
| `scripts/update_lxqt_panel.sh` | Rebuild LXQt panel from source | `bash update_lxqt_panel.sh` |

### Shared Library

`scripts/lib.sh` provides cross-distro utilities sourced by all feature scripts:

- `detect_pm()` -- identifies apt, dnf, pacman, or zypper
- `detect_distro()` -- identifies debian, fedora, rhel, suse, arch, void, alpine
- `detect_distro_version()` -- returns VERSION_ID from os-release
- `is_installed()` -- checks if a package is installed (rpm/dpkg/pacman)
- `pkg_available()` -- checks if a package exists in repos
- `pkg_install()` -- installs packages via the detected package manager
- `pkg_install_fallback()` -- tries primary name then distro-specific alternatives
- `enable_copr()` -- enables a Fedora Copr repository
- `download()` -- downloads files with resume support and caching
- `download_extract()` -- downloads and extracts tar.gz/zip archives
- `git_clone()` -- shallow-clones or updates a git repository
- `install_theme()`, `install_icons()`, `install_fonts()` -- themed install helpers

---

## Keybindings

Defined in `configs/dotfiles/labwc/rc.xml`. Super key is mapped to the Windows/Meta key.

| Binding | Action |
|---------|--------|
| Super+Enter | Launch terminal (qterminal) |
| Super+Shift+Enter | Launch dropdown terminal |
| Super+D | Toggle show desktop |
| Super+E | Launch file manager (pcmanfm-qt) |
| Super+Q | Close active window |
| Super+M | Toggle maximize |
| Super+F | Toggle fullscreen |
| Super+X | Toggle window decorations |
| Super+V | Open audio mixer (pavucontrol) |
| Super+L | Session logout (lxqt-leave) |
| Super+Shift+E | Open appearance config |
| Super+Shift+X | Open session config |
| Super+Shift+R | Reconfigure compositor |
| Alt+F2 | Application launcher (rofi or lxqt-runner) |
| Alt+Space | Application launcher |
| Ctrl+Alt+T | Launch terminal |
| Ctrl+Alt+E | Launch Emacs |
| Super+1-4 | Switch to desktop 1-4 |
| Super+Shift+1-4 | Move window to desktop 1-4 |
| Super+Left/Right/Up/Down | Snap window to edge |
| Ctrl+Super+Left/Right/Up/Down | Snap window to region |
| Super+Arrow (move) | Move window to edge |
| Super+Tab | Next window |
| Super+Shift+Tab | Previous window |
| Print Screen | Capture full screen (grimshot copy screen) |
| Shift+Print Screen | Capture selected area (grimshot copy area) |
| Ctrl+Print Screen | Save full screen to ~/Pictures/Screenshots/ |
| Super+F12 | Lock screen (swaylock) |
| XF86AudioRaise/Lower Volume | Adjust volume (pactl +5%/-5%) |
| XF86AudioMute | Toggle mute |
| XF86AudioPlay/Next/Prev | Media player control (playerctl) |
| XF86MonBrightnessUp/Down | Adjust brightness (brightnessctl +/-5%) |

---

## Environment Variables

Defined in `configs/dotfiles/labwc/environment`. These are loaded by Labwc at session start.

| Variable | Value | Purpose |
|----------|-------|---------|
| XKB_DEFAULT_LAYOUT | us | Keyboard layout |
| XKB_DEFAULT_OPTIONS | grp:alt_shift_toggle,compose:ralt | Layout toggle, compose key |
| XCURSOR_THEME | breeze_cursors | Cursor theme |
| XCURSOR_SIZE | 24 | Cursor size |
| QT_QPA_PLATFORMTHEME | qt6ct | Qt6 configuration backend |
| QT_WAYLAND_DISABLE_WINDOWDECORATION | 1 | Use Labwc CSD, not Qt CSD |
| XDG_CURRENT_DESKTOP | LXQt:labwc:wlroots | Desktop identification |
| XDG_SESSION_DESKTOP | lxqt-labwc | Session identification |
| XDG_SESSION_TYPE | wayland | Session type |
| MOZ_ENABLE_WAYLAND | 1 | Firefox Wayland backend |
| MOZ_WEBRENDER | 1 | Firefox WebRender |
| ELECTRON_ENABLE_WAYLAND | 1 | Electron Wayland backend |
| ELECTRON_OZONE_PLATFORM_HINT | auto | Electron auto-detect |
| GDK_BACKEND | wayland,x11 | GTK backend preference |
| SDL_VIDEO_DRIVER | wayland,x11 | SDL backend preference |
| CLUTTER_BACKEND | wayland | Clutter backend |
| WLR_NO_HARDWARE_CURSORS | 0 | Hardware cursors (set 1 for VMs) |
| _JAVA_AWT_WM_NONREPARENTING | 1 | Java AWT fix for Wayland |
| GTK_USE_PORTAL | 1 | Use portal for file chooser |

---

## Theming

### Color Palette

The desktop uses a Nord-inspired palette defined in `labwc/themerc`:

| Element | Color |
|---------|-------|
| Active border | `#81a1c1` |
| Inactive border | `#2e3440` |
| Active title background | `#3b4252` |
| Active title foreground | `#eceff4` |
| Menu background | `#2e3440` |
| Menu active selection | `#81a1c1` |

### Theme Components

| Component | Config File | Theme Engine |
|-----------|-------------|--------------|
| Window decorations | labwc/themerc-override | Openbox themes (Vent-dark default) |
| Qt widgets | qt6ct/qt6ct.conf | Kvantum or Fusion |
| GTK widgets | gtk-3.0/settings.ini, gtk-4.0/settings.ini | GTK themes (Adwaita-dark default) |
| Icons | lxqt/session.conf | Breeze icons (default) |
| Cursors | lxqt/session.conf | Breeze cursors (default) |
| Panel | lxqt/panel.conf | QSS (Qt Style Sheets) |
| Notifications | dunst/dunstrc | Custom colors |
| Screen locker | swaylock/config | Custom appearance |
| Launcher | rofi/config.rasi | Nord-themed CSS |

### Applying Themes

```bash
# GTK/Qt themes
bash scripts/themes.sh --all

# Icon packs
bash scripts/icons.sh --all

# Cursor themes
bash scripts/cursors.sh --all

# Window decorations for Labwc
bash scripts/labwc-themes.sh --all

# LXQt widget themes
bash scripts/lxqt-themes.sh --all

# Apply via LXQt settings GUI
lxqt-config-appearance
```

---

## Distro Support

### Package Manager Detection

All scripts in `scripts/features/` use `lib.sh` for cross-distro compatibility. The library auto-detects:

- **apt** -- Debian, Ubuntu, Linux Mint, Pop!_OS, Kali, Devuan
- **dnf** -- Fedora, RHEL, CentOS, Rocky Linux, AlmaLinux
- **pacman** -- Arch, Manjaro, EndeavourOS, Garuda, Artix
- **zypper** -- openSUSE, SLES

### Fedora/RHEL Specific

Enable required repositories before building:

```bash
bash scripts/features/setup-fedora-repos.sh
```

This enables:
- EPEL (Extra Packages for Enterprise Linux) on RHEL/CentOS/Rocky/Alma
- CRB or PowerTools repository for development headers
- Copr repositories for packages not in default repos (labwc, etc.)

The build script auto-detects Fedora and enables the `zhsj/labwc` Copr repository.

### Package Name Mapping

Some packages have different names across distros:

| Component | apt (Debian/Ubuntu) | dnf (Fedora/RHEL) | pacman (Arch) |
|-----------|--------------------|--------------------|---------------|
| Build tools | build-essential | "Development Tools" group | base-devel |
| Qt6 dev | qt6-base-dev | qt6-qtbase-devel | qt6-base |
| KF6 Window System | libkf6windowsystem-dev | kf6-kwindowsystem-devel | extra-cmake-modules |
| Polkit Qt6 | libpolkit-qt6-1-dev | polkit-qt6-1-devel | polkit-qt6 |
| D-Bus menu | libdbusmenu-glib-dev | libdbusmenu-glib-devel | libdbusmenu-glib |
| Emacs | emacs-gtk | emacs | emacs |
| Screenshots | grim (grimshot) | grim (grimshot) | grim (grimshot) |

---

## Troubleshooting

### Panel does not show window buttons

The panel requires the compositor's `wlr-foreign-toplevel` protocol. Labwc supports this natively. If using KWin, set `LXQT_PANEL_WAYLAND_BACKEND=KWindowSystem` before launching the panel.

### Qt applications look wrong

Ensure `QT_QPA_PLATFORMTHEME=qt6ct` is set in the environment file. Run `qt6ct` to configure the Qt6 style and font. If using Kvantum, set `QT_STYLE_OVERRIDE=kvantum` and configure via Kvantum Manager.

### No system tray icons

Add both the "Status Notifier" and "System Tray" plugins to the panel. Status Notifier handles modern SNI protocol apps (Discord, Telegram). System Tray handles legacy XEmbed apps.

### Clipboard history not working

Verify `wl-paste --watch cliphist store` is running in the autostart. Check with:

```bash
pgrep -a cliphist
```

If not running, manually start it:

```bash
wl-paste --type text --watch cliphist store &
wl-paste --type image --watch cliphist store &
```

### Screen sharing does not work

Install and verify the portal stack:

```bash
bash scripts/features/setup-portals.sh
```

Ensure PipeWire is running:

```bash
systemctl --user status pipewire
```

### GTK applications have wrong cursor

The autostart syncs cursor settings from `session.conf` to GTK via `gsettings`. If the cursor is wrong, re-run the autostart or manually set:

```bash
gsettings set org.gnome.desktop.interface cursor-theme 'breeze_cursors'
gsettings set org.gnome.desktop.interface cursor-size 24
```

### Reset panel to stock

```bash
bash scripts/reset-panel.sh
labwc -r
```

To restore from backup:

```bash
bash scripts/reset-panel.sh --restore
labwc -r
```

### Session does not start

Switch to a TTY (Ctrl+Alt+F2), log in, and check `~/.config/labwc/environment` for correct `XDG_CURRENT_DESKTOP` and `XDG_SESSION_TYPE` values. Verify Labwc is installed:

```bash
which labwc
labwc --version
```

---

## License

This project provides configuration files and scripts for the LXQt desktop environment. Individual components (LXQt, Labwc, Qt6) retain their original licenses. Configuration files in this repository are provided as-is for personal use.
