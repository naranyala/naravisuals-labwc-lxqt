# Applications

Reference for all Qt6 GUI applications in the suite.

## Control Center

**Binary:** `apps/control-center/build/nv-control-center`
**Build:** `cmake -GNinja .. && ninja` in `apps/control-center/build/`
**Dependencies:** Qt6::Core, Qt6::Gui, Qt6::Widgets

### Pages

| Page | Purpose | Buttons |
|------|---------|---------|
| Appearance | SDDM, themes, icons, cursors, fonts | SDDM Theme selector, Fix GTK Controls, Install System Themes, Install Icon Packs, Install Cursor Themes, Install Custom Fonts |
| Desktop and WM | Compositor, panel management | Switch LXQt Compositor, Install LXQt Themes, Install Labwc Themes, Reset Panel to Stock |
| Wallpapers | Wallpaper management | Update SDDM Wallpaper, Set Default Desktop Wallpaper, Download Extra Wallpapers |
| Addons and Apps | Widgets, disk, maintenance | NTFS Partition Manager, Install Conky, Install Neofetch, Install Emacs Config, Update LXQt Panel from Source |
| Input Devices | Touchpad settings | Enable Natural Scrolling, Enable Touchpad Tap-to-Click |
| Compositors | Multi-compositor install | Install Hyprland, Install Sway, Install Wayfire, Switch Compositor |
| Theming | Kvantum, wallust, panel | Install Kvantum + Qt6CT, Install Wallust, Apply Colors to Current Wallpaper, Reset Panel to Stock |
| System | Portals, Fedora, maintenance | Setup XDG Portals, Setup Fedora Repos, Update LXQt Panel from Source |

### Architecture

The Control Center is a `QMainWindow` with a `QListWidget` sidebar and `QStackedWidget` pages. Each button calls `runScript()` or `runTerminalScript()` which dispatches to Bash scripts via `QProcess::startDetached()`.

```
ControlCenterWindow
  +-- QListWidget (sidebar, 8 items)
  +-- QStackedWidget (8 pages)
        +-- AppearancePage
        +-- DesktopPage
        +-- WallpaperPage
        +-- AddonsPage
        +-- InputPage
        +-- CompositorPage
        +-- ThemingPage
        +-- SystemPage
```

### Styling

Catppuccin-inspired dark theme:
- Background: `#1e1e2e`
- Text: `#cdd6f4`
- Accent: `#89b4fa`
- Group border: `#45475a`
- Button: `#89b4fa` on `#1e1e2e`

---

## SDDM GUI

**Binary:** `apps/sddm-gui/build/sddm-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/sddm-gui/build/`

### Features

- Lists installed SDDM themes from `/usr/share/sddm/themes/`
- Dropdown selector for theme selection
- "Apply Theme" button writes to `/etc/sddm.conf.d/custom.conf` via `pkexec`

### Architecture

Single `QWidget` with `QComboBox` for theme selection and `QPushButton` for apply. Uses `pkexec` for privilege escalation when writing to `/etc/sddm.conf.d/`.

---

## NTFS GUI

**Binary:** `apps/ntfs-gui/build/nv-ntfs-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/ntfs-gui/build/`

### Features

- Scans all partitions on startup via `lsblk`
- Displays Device, Label, FS Type, Size, Status, Mount Point in a table
- "Mount" button uses `pkexec bash -c "mkdir -p ... && mount ..."`
- "Unmount" button uses `pkexec bash -c "umount ..."`
- "Refresh" button re-scans partitions
- Mount point: `/media/<user>/<label-or-uuid>`

### Architecture

`QMainWindow` with `QTableWidget` for partition display and `QPushButton` for actions. All mount/unmount operations go through `pkexec` for privilege escalation.

### Dependencies

- `lsblk` — partition scanning
- `blkid` — label and UUID lookup
- `findmnt` — mount point detection
- `pkexec` — privilege escalation

---

## Audio Manager

**Binary:** `apps/audio-gui/build/nv-audio-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/audio-gui/build/`

### Features

- Auto-detects PipeWire (wpctl) or PulseAudio (pactl) backend
- Lists output devices with volume slider and mute toggle
- Lists input devices with mute toggle
- Switch active output/input device
- CLI mode: `--list` dumps device status

### Dependencies

- `wpctl` (PipeWire) or `pactl` (PulseAudio)

---

## Display Manager

**Binary:** `apps/display-gui/build/nv-display-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/display-gui/build/`

### Features

- Parses `wlr-randr` output to detect connected displays
- Resolution and refresh rate selector per output
- Orientation selector (normal, 90, 180, 270, flipped variants)
- Enable/disable individual outputs
- CLI mode: `--list` dumps display status

### Dependencies

- `wlr-randr`

---

## Wallpaper Manager

**Binary:** `apps/wallpaper-gui/build/nv-wallpaper-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/wallpaper-gui/build/`

### Features

- Auto-detects swww or swaybg backend
- Scans wallpaper directories (recursive, 1 level deep)
- Thumbnail grid with preview
- One-click apply via swww or swaybg
- CLI mode: `--set <path>` applies wallpaper headlessly

### Dependencies

- `swww` or `swaybg`

---

## Input Devices Manager

**Binary:** `apps/input-gui/build/nv-input-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/input-gui/build/`

### Features

- Parses `libinput list-devices` output
- Shows pointer and keyboard devices
- Natural scroll toggle, pointer speed slider per device
- CLI mode: `--list` dumps device info

### Dependencies

- `libinput`

---

## Power Management

**Binary:** `apps/power-gui/build/nv-power-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/power-gui/build/`

### Features

- Configures swayidle screen blank and lock timeouts
- Lid close action selector
- Generates swayidle config file
- Reads existing config on load

### Dependencies

- `swayidle`, `swaylock`

---

## Bluetooth Manager

**Binary:** `apps/bluetooth-gui/build/nv-bluetooth-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/bluetooth-gui/build/`

### Features

- Lists paired Bluetooth devices with name, MAC, type, status
- Pair & connect new devices
- Disconnect paired devices
- CLI mode: `--list` dumps device list

### Dependencies

- `bluetoothctl` (bluez)

---

## WiFi Manager

**Binary:** `apps/network-gui/build/nv-network-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/network-gui/build/`

### Features

- Scans available WiFi networks via nmcli
- Shows SSID, signal strength, security, active connection
- Connect with password, disconnect
- CLI mode: `--list` dumps network list

### Dependencies

- `nmcli` (NetworkManager)

---

## Systemd Service Manager

**Binary:** `apps/service-gui/build/nv-service-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/service-gui/build/`

### Features

- Lists all systemd user services
- Start, stop, enable, disable services
- Shows load/active/sub states
- CLI mode: `--list` dumps service list

### Dependencies

- `systemctl`

---

## System Log Viewer

**Binary:** `apps/log-gui/build/nv-log-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/log-gui/build/`

### Features

- Views journalctl logs with priority filter
- Text search/grep filter
- Follow (live) mode with real-time log streaming
- Configurable line count
- CLI mode: `--last` dumps last 50 lines

### Dependencies

- `journalctl`

---

## Disk Information

**Binary:** `apps/disk-gui/build/nv-disk-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/disk-gui/build/`

### Features

- Lists partitions via lsblk (device, type, size, mount, label)
- SMART health check via smartctl
- CLI mode: `--list` dumps partition table

### Dependencies

- `lsblk`, `smartctl` (smartmontools, optional)

---

## Theme Manager

**Binary:** `apps/theme-gui/build/nv-theme-gui`
**Build:** `cmake -GNinja .. && ninja` in `apps/theme-gui/build/`

### Features

- Scans Qt/Kvantum themes from ~/.config/Kvantum and /usr/share/Kvantum
- Scans GTK themes from /usr/share/themes and ~/.themes
- Scans icon themes from /usr/share/icons and ~/.icons
- Apply Qt theme via qt6ct config
- Apply GTK and icon themes via gsettings
- Shows current active theme per category
