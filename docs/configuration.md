# Configuration Reference

Complete reference for every configuration file in the suite.

## Labwc Compositor

### rc.xml — Keybindings and Window Rules

**Location:** `configs/dotfiles/labwc/rc.xml` → `~/.config/labwc/rc.xml`

Controls all keyboard shortcuts, mouse bindings, window rules, and input device settings.

**Sections:**

| Section | Purpose |
|---------|---------|
| `<core>` | Global compositor settings (gap, focus, decoration) |
| `<desktops>` | Virtual desktop count and names |
| `<theme>` | Window decoration theme, fonts, corner radius |
| `<keyboard>` | All keybindings |
| `<mouse>` | Mouse bindings per context |
| `<libinput>` | Touchpad and mouse configuration |
| `<windowRules>` | Per-app window rules |

**Key settings:**

```xml
<core>
  <gap>8</gap>                    <!-- Gap between windows -->
  <focusMode>sloppy</focusMode>   <!-- Focus follows mouse -->
  <cornerRadius>10</cornerRadius> <!-- Window corner rounding -->
</core>
```

### autostart — Session Startup Daemons

**Location:** `configs/dotfiles/labwc/autostart` → `~/.config/labwc/autostart`

Shell script executed by Labwc at session start. Launches daemons conditionally.

**Services launched:**

| Service | Condition | Purpose |
|---------|-----------|---------|
| swaybg | `command -v swaybg` | Wallpaper rendering |
| dunst | `command -v dunst` | Notifications |
| lxqt-policykit-agent | `command -v lxqt-policykit-agent` | Privilege escalation |
| cliphist | `command -v cliphist && wl-paste` | Clipboard history |
| swayidle | `command -v swayidle` | Screen idle management |
| nm-applet | `command -v nm-applet` | Network tray |
| blueman-applet | `command -v blueman-applet` | Bluetooth tray |
| xdg-desktop-portal | `command -v xdg-desktop-portal` | Portal daemon |

### environment — Session Environment Variables

**Location:** `configs/dotfiles/labwc/environment` → `~/.config/labwc/environment`

Loaded by Labwc at startup. Controls Wayland, Qt, GTK, and browser behavior.

**Critical variables:**

| Variable | Value | Controls |
|----------|-------|----------|
| `XDG_CURRENT_DESKTOP` | `LXQt:labwc:wlroots` | Desktop identification |
| `XDG_SESSION_TYPE` | `wayland` | Session type |
| `QT_QPA_PLATFORMTHEME` | `qt6ct` | Qt6 configuration backend |
| `QT_WAYLAND_DISABLE_WINDOWDECORATION` | `1` | Use Labwc CSD |
| `MOZ_ENABLE_WAYLAND` | `1` | Firefox Wayland backend |
| `ELECTRON_ENABLE_WAYLAND` | `1` | Electron Wayland backend |
| `GDK_BACKEND` | `wayland,x11` | GTK backend preference |
| `GTK_USE_PORTAL` | `1` | Use portal for file chooser |
| `XCURSOR_THEME` | `breeze_cursors` | Cursor theme |
| `XCURSOR_SIZE` | `24` | Cursor size |

### themerc — Window Decoration Theme

**Location:** `configs/dotfiles/labwc/themerc` → `~/.config/labwc/themerc-override`

Openbox-compatible theme for window decorations. Nord palette.

| Property | Value |
|----------|-------|
| Active border | `#81a1c1` |
| Inactive border | `#2e3440` |
| Active title bg | `#3b4252` |
| Active title fg | `#eceff4` |
| Menu background | `#2e3440` |
| Menu active | `#81a1c1` |

### menu.xml — Root and Client Menus

**Location:** `configs/dotfiles/labwc/menu.xml` → `~/.config/labwc/menu.xml`

Right-click menu content. Defines applications and actions in the root menu.

---

## LXQt Panel

### panel.conf — Panel Layout and Plugins

**Location:** `configs/dotfiles/lxqt/panel.conf` → `~/.config/lxqt/panel.conf`

INI format. Defines panel position, size, plugins, and per-plugin settings.

**Structure:**

```ini
[General]
panels=panel1                          # Active panels

[panel1]
position=Bottom                        # Top, Bottom, Left, Right
plugins=fancymenu,desktopswitch,taskbar,spacer,statusnotifier,tray,volume,clock,showdesktop
height=48
background-color=rgba(30,30,30,230)    # Semi-transparent dark

[fancymenu]
type=fancymenu
showText=true
text=Applications

[taskbar]
type=taskbar
showOnlyCurrentDesktopTasks=true
maxButtonWidth=250

[volume]
type=volume
mixer=pavucontrol
volumeStep=5

[clock]
type=clock
clockFmt=ddd MMM d HH:mm
```

### panel-stock.conf — Stock Panel Reference

**Location:** `configs/dotfiles/lxqt/panel-stock.conf`

Reference config used by `reset-panel.sh`. Plugins: fancymenu, desktopswitch, taskbar, spacer, statusnotifier, tray, volume, clock, showdesktop.

### lxqt-panel.qss — Panel QSS Theme

**Location:** `configs/dotfiles/lxqt/lxqt-panel.qss` → `~/.config/lxqt/lxqt-panel.qss`

Qt Style Sheet for panel plugins. Nord palette with Catppuccin-inspired accent colors.

**Styled elements:**
- `LXQtTaskButton` — Taskbar window buttons
- `LXQtTrayButton` — System tray icons
- `LXQtVolume` — Volume plugin
- `LXQtClock` — Clock plugin
- `LXQtDesktopSwitch` — Desktop pager
- `LXFancyMenu` — Application menu
- `LXQtShowDesktop` — Show desktop button

---

## GTK Integration

### gtk-3.0/settings.ini

**Location:** `configs/dotfiles/gtk-3.0/settings.ini` → `~/.config/gtk-3.0/settings.ini`

```ini
[Settings]
gtk-theme-name=Adwaita-dark
gtk-icon-theme-name=breeze
gtk-cursor-theme-name=breeze_cursors
gtk-cursor-theme-size=24
gtk-font-name=Noto Sans 10
gtk-application-prefer-dark-theme=true
gtk-decoration-button-layout=menu:minimize,maximize,close
```

### gtk-4.0/settings.ini

Same structure as GTK3, applied to GTK4 applications.

---

## Application Launcher

### rofi/config.rasi

**Location:** `configs/dotfiles/rofi/config.rasi` → `~/.config/rofi/config.rasi`

Rofi configuration with Nord-themed CSS. Supports `drun`, `run`, and `window` modes.

**Theme colors:**
- Background: `#2e3440F2`
- Accent: `#81a1c1`
- Text: `#d8dee9`
- Selected: `#81a1c1F2`

---

## Notification Daemon

### dunst/dunstrc

**Location:** `configs/dotfiles/dunst/dunstrc` → `~/.config/dunst/dunstrc`

Notification appearance and behavior. Frame color and urgency levels match Nord palette.

---

## Screen Locker

### swaylock/config

**Location:** `configs/dotfiles/swaylock/config` → `~/.config/swaylock/config`

Screen locker appearance. Launched by swayidle on idle timeout.

---

## OSD Overlay

### wob/config

**Location:** `configs/dotfiles/wob/config` → `~/.config/wob/config`

Volume/brightness progress bar overlay.

```ini
width=600
height=30
margin=20
border_color=81a1c1FF
background_color=2e3440DD
bar_color=81a1c1FF
```

---

## Clipboard History

### cliphist/config

**Location:** `configs/dotfiles/cliphist/config` → `~/.config/cliphist/config`

Clipboard history manager configuration. Used with `wl-paste --watch cliphist store`.

---

## Output Management

### kanshi/config

**Location:** `configs/dotfiles/kanshi/config` → `~/.config/kanshi/config`

Auto-configures display outputs on hotplug. Defines profiles for different monitor arrangements.

---

## XDG Portal

### xdg-desktop-portal-wlr/config

**Location:** `configs/dotfiles/xdg-desktop-portal-wlr/config` → `~/.config/xdg-desktop-portal-wlr/config`

Portal backend for wlroots compositors. Required for screen sharing.

---

## Compositor Profiles

### Hyprland

**Location:** `configs/compositors/hyprland/hyprland.conf` → `~/.config/hypr/hyprland.conf`

Full config with animations, blur, shadows, keybinds, input settings, and LXQt autostart.

### Sway

**Location:** `configs/compositors/sway/config` → `~/.config/sway/config`

i3-compatible tiling config with Nord colors, gaps, and LXQt autostart.

### Wayfire

**Location:** `configs/compositors/wayfire/wayfire.ini` → `~/.config/wayfire/wayfire.ini`

Plugin-based compositor with decoration, blur, shadows, and animations.
