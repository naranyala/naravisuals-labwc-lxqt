# Compositor Guide

Multi-compositor support for LXQt on Wayland.

## Supported Compositors

| Compositor | Type | Config Location | Package |
|------------|------|-----------------|---------|
| Labwc | Stacking (default) | `~/.config/labwc/` | labwc |
| Hyprland | Tiling | `~/.config/hypr/hyprland.conf` | hyprland |
| Sway | Tiling (i3-compatible) | `~/.config/sway/config` | sway |
| Wayfire | 3D plugin-based | `~/.config/wayfire/wayfire.ini` | wayfire |

## Installing a Compositor

```bash
# Install Hyprland
bash scripts/features/install-compositor.sh hyprland

# Install Sway
bash scripts/features/install-compositor.sh sway

# Install Wayfire
bash scripts/features/install-compositor.sh wayfire

# List available profiles
bash scripts/features/install-compositor.sh --list
```

Each installer:
1. Installs the compositor package via the detected package manager
2. Copies the project's config profile to `~/.config/<compositor>/`
3. Updates `~/.config/lxqt/session.conf` with the `window_manager=` field

## Switching Compositors

```bash
bash scripts/switch-lxqt-compositor.sh
```

This script:
1. Installs any missing compositor packages
2. Presents a numbered menu of available compositors
3. Stops all running compositors and LXQt services
4. Starts the selected compositor
5. Restarts LXQt services

## Per-Compositor Details

### Labwc (Default)

- **Type:** Stacking window manager, Openbox-inspired
- **Config:** XML-based (`rc.xml`, `menu.xml`)
- **Keybindings:** Defined in `rc.xml` `<keyboard>` section
- **Theme:** Openbox themes via `themerc-override`
- **Best for:** Users who want a traditional desktop experience

**Unique features:**
- Openbox theme parsing
- XML keybindings with modifier combinations
- Root and client menus
- libinput device configuration

### Hyprland

- **Type:** Dynamic tiling with animations
- **Config:** INI-like format (`hyprland.conf`)
- **Keybindings:** Defined via `bind` and `bindm` directives
- **Theme:** Built-in blur, shadows, animations
- **Best for:** Users who want a modern, animated tiling experience

**Unique features:**
- Gaussian blur on windows
- Smooth window animations
- Per-window opacity rules
- Master/stack layout

### Sway

- **Type:** Static tiling (i3-compatible)
- **Config:** i3-compatible format (`config`)
- **Keybindings:** Defined via `bindsym` directives
- **Theme:** Minimal, relies on external tools
- **Best for:** Users who want i3-style tiling on Wayland

**Unique features:**
- i3 IPC compatibility
- Scriptable configuration
- Extensive plugin ecosystem
- Gap support via `gaps` directive

### Wayfire

- **Type:** 3D plugin-based compositor
- **Config:** INI format (`wayfire.ini`)
- **Keybindings:** Defined in config sections
- **Theme:** Plugin-based effects
- **Best for:** Users who want 3D effects and plugin extensibility

**Unique features:**
- Plugin architecture (blur, animation, scale, cube)
- 3D window transformations
- Expo view
- Switcher animations

## Compositor Comparison

| Feature | Labwc | Hyprland | Sway | Wayfire |
|---------|-------|----------|------|---------|
| Window management | Stacking | Dynamic tiling | Static tiling | Stacking |
| Animations | No | Yes | No | Yes (plugin) |
| Blur | No | Yes | No | Yes (plugin) |
| Rounded corners | Yes | Yes | No | Yes |
| Gaps | Yes | Yes | Yes | Yes |
| Screen sharing | Yes | Yes | Yes | Yes |
| XWayland | Yes | Yes | Yes | Yes |
| Config format | XML | INI | i3-style | INI |
| LXQt integration | Native | Manual | Manual | Manual |

## Session Configuration

The active compositor is set in `~/.config/lxqt/session.conf`:

```ini
window_manager=labwc    # or hyprland, sway, wayfire
```

The install-compositor.sh script updates this field automatically.
