# Naravisuals Desktop Environment Suite

The Naravisuals Desktop Environment Suite provides a comprehensive, production-ready configuration and customization layer for an LXQt desktop session running atop the Labwc Wayland compositor. It integrates highly customized dotfiles, native C++ deployment tools, Qt6 management applications, and robust automation scripts to deliver a streamlined, modern Linux desktop experience.

## Architecture and Abstraction Layers

This repository is structured into distinct abstraction layers to separate logic, configuration, and interface code.

### 1. `apps/` - Native Management Utilities
Contains custom C++ Qt6 Graphical User Interfaces designed to manage system configurations securely and interactively.
*   **Control Center**: A unified settings hub with a modern interface for managing appearance, session components, input devices, and system add-ons.
*   **SDDM GUI**: A dedicated graphical interface for parsing and modifying the Simple Desktop Display Manager (SDDM) configuration.

### 2. `cmd/` - Deployment Tooling
Contains native C++ applications for system deployment and configuration management.
*   **Dotfiles Installer**: A robust, cross-platform C++ binary that installs, lists, and validates dotfiles into the user's `~/.config` directory without requiring manual symbolic links.

### 3. `configs/` - Core Configurations
Contains the static configuration files and dotfiles required for the desktop ecosystem.
*   **LXQt**: Session definitions, panel plugins, layout specifications, and appearance settings.
*   **Labwc**: Compositor behavior, XML keybindings, root/client menus, autostart daemons, environment variables, and window theme overrides (Nord palette).
*   **GTK Integration**: Universal settings for GTK-3.0 and GTK-4.0 to enforce dark themes and consistent cursor styling across environments.

### 4. `scripts/` - Automation and Administration
Contains Bash automation scripts separated by purpose.
*   **build/**: Comprehensive source compilation scripts (e.g., fetching, configuring, and compiling a lightweight Qt6 environment and LXQt core components from master).
*   **features/**: Standalone administrative scripts for adjusting hardware input behavior, fixing GTK window control layouts, managing wallpapers, and installing third-party system themes.

### 5. `assets/` - Media and Resources
Contains static assets utilized by the configuration files.
*   Default system wallpapers.
*   Inspirational design mockups and visual reference material.

## Installation and Deployment

### 1. Environment Compilation
To build the latest available LXQt desktop environment directly from source using a customized, lightweight Qt6 build, execute the provided build script with elevated privileges:

```bash
sudo ./scripts/build/build_latest_lxqt_desktop.sh
```

### 2. Dotfiles Installation
To deploy the configurations to your local user environment, compile and execute the native C++ installer:

```bash
cd cmd/dotfiles-manager
c++ -std=c++17 build.cpp -o lxqt-dotfiles
./lxqt-dotfiles install
```

### 3. Management GUI Compilation
To compile the Control Center for managing the deployed desktop:

```bash
cd apps/control-center
mkdir -p build && cd build
cmake -GNinja ..
ninja
./naravisuals-control-center
```

## Desktop Features

### Window Management and Compositing
*   **Labwc Integration**: Engineered for wlroots, providing a robust Wayland session with full support for Qt6 and GTK.
*   **Window Snapping**: Super+Arrow combinations for edge snapping; Ctrl+Super+Arrow combinations for regional snapping.
*   **Input Management**: Built-in support for touchpad tap-to-click, natural scrolling, and dynamic disable-while-typing logic.
*   **Dropdown Terminal**: Pre-configured global dropdown terminal accessible via F12.

### System Theming
The desktop utilizes a curated Nord-inspired color palette for minimal eye strain and consistent aesthetic rendering across toolkits.

| Element | Color Value |
|---------|-------------|
| Active Border | `#81a1c1` |
| Inactive Border | `#2e3440` |
| Active Title Background | `#3b4252` |
| Active Title Foreground | `#eceff4` |
| Menu Background | `#2e3440` |
| Menu Active Selection | `#81a1c1` |

### Keybindings Reference

| Binding | Action |
|---------|--------|
| Super+Enter | Launch Terminal |
| Super+D | Toggle Desktop Visibility |
| Super+E | Launch File Manager |
| Super+Q | Close Active Window |
| Super+F | Toggle Fullscreen |
| Super+V | Open Audio Mixer |
| Super+L | Session Logout |
| Alt+F2 | Launch Application Runner |
| Super+Shift+R | Reconfigure Compositor |
| Print Screen | Capture Full Screen |
| Shift+Print Screen | Capture Selected Area |

## Dependencies and Requirements

*   **Core Systems**: LXQt (>= 2.1), Labwc (>= 0.7.1), Qt6 Toolchain
*   **Wayland Components**: swaybg (wallpapers), swaylock (security), swayidle (power management)
*   **Theming**: breeze-icon-theme, breeze-cursor-theme
*   **Optional Utilities**: dunst (notifications), copyq (clipboard management), grim/slurp (screenshots)

## Maintenance

Development on this repository follows strict abstraction principles. Any new system modifications should be written as scripts in `scripts/features/` and integrated into the `apps/control-center/` interface. Standalone configurations must be routed through the Go installation tooling in `cmd/`.
