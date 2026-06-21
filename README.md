# LXQt Labwc Dotfiles

Rich dotfiles configuration for **LXQt 2.x** desktop environment using the **labwc** Wayland compositor.

## Overview

This project provides a comprehensive, production-ready dotfiles setup for an LXQt + Labwc desktop session. It includes configuration for the LXQt panel, labwc compositor, GTK/Qt theming, and session management.

## Directory Structure

```
dotfiles/
├── lxqt/
│   ├── session.conf           LXQt session settings (keyboard, cursor, env)
│   ├── panel.conf             LXQt panel plugins, layout, appearance
│   ├── lxqt.conf              General Qt/LXQt application settings
│   └── lxqt-config.conf       Configuration center window state
├── labwc/
│   ├── rc.xml                 Compositor config: keybinds, mouse, themes, libinput
│   ├── menu.xml               Application menus (root-menu, client-menu)
│   ├── autostart              Session autostart (wallpaper, daemons, services)
│   ├── environment            Environment variables (Wayland, Qt, GTK, XKB)
│   ├── themerc                Window theme overrides (Nord-inspired palette)
│   └── shutdown               Cleanup script on session exit
├── gtk-3.0/
│   └── settings.ini           GTK3 dark theme, fonts, cursor
├── gtk-4.0/
│   └── settings.ini           GTK4 dark theme, fonts, cursor
└── qt6ct/
    └── qt6ct.conf             Qt6 appearance (Fusion style, dark palette)
```

## Quick Start

```bash
# Build and install dotfiles to ~/.config/
go build -o lxqt-dotfiles .
./lxqt-dotfiles install

# Or use the shell install script:
bash install.sh

# Note: For system-level integration (display manager and wayland session), you must manually copy these files as root:
sudo cp dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/
sudo cp dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/
```

### Commands

| Command | Description |
|---------|-------------|
| `install` | Install all dotfiles to `~/.config/` |
| `install --dry-run` | Preview what would be installed |
| `install -f` | Overwrite existing files |
| `install --home /path` | Install to a custom home directory |
| `list` | List all managed dotfiles |
| `validate` | Verify installed files match embedded ones |
| `version` | Show version |

## Features

### LXQt Panel
- Bottom panel with: app menu, desktop switcher, quick launch, taskbar, system tray, volume, clock, show desktop
- Dark transparent background (rgba 30,30,30,230)
- Wayland backend configured for labwc
- 32px icon size, 48px panel height

### labwc Compositor
- **Keybindings**: Win+Enter (terminal), Win+d (show desktop), Win+e (file manager), Win+q (close), Alt+F2/Space (runner)
- **Snapping**: Win+arrows snap to edge, Ctrl+Win+arrows snap to region
- **Workspaces**: 4 workspaces, Win+1-4 to switch, Win+Shift+1-4 to move windows
- **Media keys**: Volume, brightness, playback controls
- **Screenshots**: Print (screen), Shift+Print (area), Ctrl+Print (save to file)
- **Mouse**: Root menu on all 3 buttons, window move/resize, titlebar shade on double-click
- **Touchpad**: Tap-to-click, two-finger scroll, tap-button-map LRM, disable-while-typing
- **Nord theme** colors via themerc-override
- Window rules for lxqt-panel, pcmanfm, qterminal dropdown

### Environment
- XKB keyboard layout with Alt+Shift toggle and Compose key
- Wayland flags for Firefox, Electron, Java, GDK, SDL
- XDG desktop set to `LXQt:labwc:wlroots`
- **Emacs**: Minimal, beginner-friendly setup out of the box using Wayland-native PGTK build, featuring modern completions and Nord theme.

### Autostart
- Wallpaper via swaybg
- Notification daemon (dunst/lxqt-notificationd)
- PolicyKit agent for privilege elevation
- Clipboard manager (copyq/cliphist)
- Screen idle/power management via swayidle
- Network manager applet
- Bluetooth tray
- GTK portals

## Requirements

- LXQt >= 2.1
- labwc >= 0.7.1
- Qt6
- swaybg (for wallpaper)
- swaylock (for screen lock)
- swayidle + wlopm (for power management)
- breeze-icon-theme (for icons)
- breeze-cursor-theme (for cursor)
- sddm (recommended display manager)
- emacs-pgtk (for minimal beginner-friendly editor GUI)

### Optional

| Package | Purpose |
|---------|---------|
| dunst / lxqt-notificationd | Notification daemon |
| copyq / cliphist + wl-paste | Clipboard manager |
| nm-applet | Network manager tray |
| blueman-applet | Bluetooth tray |
| pavucontrol | Audio mixer |
| grimshot / grim + slurp | Screenshots |
| playerctl | Media key controls |
| brightnessctl | Backlight controls |
| labwc-tweaks | GUI labwc configuration |
| kanshi | Display/output management |
| wlr-randr | Display configuration |

## Keybindings Reference

| Key | Action |
|-----|--------|
| Win+Enter | Terminal |
| Win+d | Show desktop |
| Win+e | File manager |
| Win+q | Close window |
| Win+m | Toggle maximize |
| Win+f | Toggle fullscreen |
| Win+v | Audio mixer |
| Win+l | Session logout |
| Win+Left/Right/Up/Down | Snap to edge |
| Win+1-4 | Switch workspace |
| Win+Shift+1-4 | Send to workspace |
| Ctrl+Win+arrows | Snap to region |
| Alt+F2 / Alt+Space | Application runner |
| Win+Shift+r | Reconfigure labwc |
| Print | Screenshot (screen) |
| Shift+Print | Screenshot (area) |
| Win+F12 | Lock screen |
| F12 | Dropdown terminal |

## Color Scheme

Window theming uses a **Nord** palette:

| Element | Color |
|---------|-------|
| Active border | `#81a1c1` |
| Inactive border | `#2e3440` |
| Active title bg | `#3b4252` |
| Active title fg | `#eceff4` |
| Menu bg | `#2e3440` |
| Menu active | `#81a1c1` |
| Menu separator | `#4c566a` |
| Close button | `#bf616a` |
| Maximize button | `#a3be8c` |
| Minimize button | `#ebcb8b` |
