# Scripts Reference

Complete reference for all 30 scripts in the suite.

## Shared Library

### lib.sh

**Location:** `scripts/lib.sh`

Core library sourced by all feature scripts. Provides:

**Logging:**
- `log_info()` / `log_ok()` / `log_warn()` / `log_error()` / `log_step()` / `log_dim()`
- All functions write to both stdout and `$_LOG_FILE`

**Error Handling:**
- `die()` — Fatal error with cleanup
- `on_cleanup()` — Register cleanup handlers
- `confirm()` — Interactive Y/n prompt
- `retry()` — Exponential backoff retry

**Pre-flight:**
- `check_prereqs()` — Full environment validation
- `check_network()` — Connectivity test
- `check_disk()` — Disk space check
- `check_sudo()` — Sudo access check

**Package Management:**
- `detect_pm()` — Returns apt, dnf, pacman, or zypper
- `detect_distro()` — Returns debian, fedora, rhel, suse, arch, void, alpine
- `detect_distro_version()` — Returns VERSION_ID
- `is_installed()` — Cross-distro package check
- `pkg_available()` — Check if package exists in repos
- `pkg_install()` — Install via detected package manager
- `pkg_install_fallback()` — Try primary name then alternatives
- `enable_copr()` — Enable Fedora Copr repo

**Downloads:**
- `download()` — Download with resume and caching
- `download_extract()` — Download and extract tar.gz/zip
- `git_clone()` — Shallow clone or update

**Install Helpers:**
- `install_theme()` — Install to ~/.themes
- `install_icons()` — Install to ~/.icons
- `install_fonts()` — Install to ~/.local/share/fonts

---

## Orchestrator

### install-all.sh

**Location:** `scripts/install-all.sh`

Module orchestrator that runs install scripts in sequence.

```bash
bash install-all.sh               # Interactive (ask for each)
bash install-all.sh --all         # Install everything
bash install-all.sh --select      # Choose which to run
bash install-all.sh --dry-run     # Preview only
bash install-all.sh themes icons  # Run specific modules
```

**20 modules:**

| Module | Script | Description |
|--------|--------|-------------|
| Fedora/RHEL Repos | features/setup-fedora-repos.sh | Enable EPEL, CRB, Copr |
| Clipboard Manager | features/install-clipboard.sh | wl-clipboard + cliphist |
| OSD Overlay | features/install-osd.sh | wob for volume/brightness |
| App Launcher | features/install-launcher.sh | rofi-wayland |
| XDG Portals | features/setup-portals.sh | Screen sharing portals |
| Kvantum Themes | features/install-kvantum.sh | SVG Qt theming |
| Dynamic Theming | features/wallust-setup.sh | Wallpaper color extraction |
| Hyprland | features/install-compositor.sh hyprland | Tiling compositor |
| Sway | features/install-compositor.sh sway | i3-compatible tiling |
| Wayfire | features/install-compositor.sh wayfire | 3D compositor |
| Theme Downloader | themes.sh | GTK/Qt themes |
| Icon Themes | icons.sh | Desktop icon packs |
| Cursor Themes | cursors.sh | Mouse cursor themes |
| Font Installer | fonts.sh | Nerd Fonts & UI fonts |
| Wallpaper Packs | wallpapers.sh | Wallpaper collections |
| labwc Decorations | labwc-themes.sh | Window decorations |
| LXQt Widget Themes | lxqt-themes.sh | Qt palette themes |
| System Fetch | neofetch.sh | neofetch/fastfetch |
| Conky Desktop | conky.sh | System monitor configs |
| Emacs Editor | emacs.sh | Emacs + config |

---

## Build Scripts

### build_latest_lxqt_desktop.sh

**Location:** `scripts/build/build_latest_lxqt_desktop.sh`

Builds Qt6 and LXQt from source. Requires sudo.

```bash
sudo ./scripts/build/build_latest_lxqt_desktop.sh
```

Steps:
1. Detects package manager (apt/dnf)
2. Installs build dependencies
3. Downloads and compiles lightweight Qt6 (skipping webengine, 3d, multimedia)
4. Clones LXQt repository with submodules
5. Builds all LXQt components via upstream build script

On Fedora, automatically enables the `zhsj/labwc` Copr repository.

### manual_build_lxqt_desktop.sh

**Location:** `scripts/build/manual_build_lxqt_desktop.sh`

Step-by-step version of the build script for debugging.

---

## Feature Scripts

### install-clipboard.sh

**Location:** `scripts/features/install-clipboard.sh`

Installs wl-clipboard and cliphist. Adds clipboard history to labwc autostart.

```bash
bash scripts/features/install-clipboard.sh
```

### install-osd.sh

**Location:** `scripts/features/install-osd.sh`

Installs wob (Wayland Overlay Bar). Creates FIFO at `/tmp/wob_fifo`. Patches rc.xml volume/brightness keys to pipe through wob.

```bash
bash scripts/features/install-osd.sh
```

### install-launcher.sh

**Location:** `scripts/features/install-launcher.sh`

Installs rofi-wayland. Replaces lxqt-runner with rofi in rc.xml keybindings.

```bash
bash scripts/features/install-launcher.sh
```

### setup-portals.sh

**Location:** `scripts/features/setup-portals.sh`

Installs xdg-desktop-portal, xdg-desktop-portal-wlr, and xdg-desktop-portal-gtk. Adds portal startup to autostart.

```bash
bash scripts/features/setup-portals.sh
```

### setup-fedora-repos.sh

**Location:** `scripts/features/setup-fedora-repos.sh`

Enables EPEL, CRB/PowerTools, and Copr repositories on Fedora/RHEL. Verifies package availability.

```bash
bash scripts/features/setup-fedora-repos.sh
```

### install-kvantum.sh

**Location:** `scripts/features/install-kvantum.sh`

Installs Kvantum and Qt6CT. Sets `QT_STYLE_OVERRIDE=kvantum`. Installs Nord panel QSS.

```bash
bash scripts/features/install-kvantum.sh
```

### wallust-setup.sh

**Location:** `scripts/features/wallust-setup.sh`

Installs wallust (Rust pywal). Creates templates for themerc, dunst, gtk, rofi. Generates `wallust-apply` helper at `~/.local/bin/`.

```bash
bash scripts/features/wallust-setup.sh          # Install + configure
bash scripts/features/wallust-setup.sh --apply   # Apply to current wallpaper
```

### install-compositor.sh

**Location:** `scripts/features/install-compositor.sh`

Installs and configures a Wayland compositor. Updates session.conf.

```bash
bash scripts/features/install-compositor.sh hyprland
bash scripts/features/install-compositor.sh sway
bash scripts/features/install-compositor.sh wayfire
bash scripts/features/install-compositor.sh --list
```

---

## Management Scripts

### reset-panel.sh

**Location:** `scripts/reset-panel.sh`

Resets lxqt-panel to stock configuration. Creates timestamped backup before overwriting.

```bash
bash scripts/reset-panel.sh              # Interactive
bash scripts/reset-panel.sh --force      # No prompt
bash scripts/reset-panel.sh --restore    # Restore from backup
bash scripts/reset-panel.sh --dry-run    # Preview
```

### switch-lxqt-compositor.sh

**Location:** `scripts/switch-lxqt-compositor.sh`

Installs and switches between compositors. Uses lib.sh helpers for cross-distro support.

```bash
bash scripts/switch-lxqt-compositor.sh
```

---

## Theme Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| themes.sh | Download GTK/Qt themes | `bash themes.sh --all` |
| icons.sh | Download icon packs | `bash icons.sh --all` |
| cursors.sh | Download cursor themes | `bash cursors.sh --all` |
| fonts.sh | Install Nerd Fonts | `bash fonts.sh --all` |
| wallpapers.sh | Download wallpaper packs | `bash wallpapers.sh --all` |
| labwc-themes.sh | Install Labwc decorations | `bash labwc-themes.sh --all` |
| lxqt-themes.sh | Install LXQt widget themes | `bash lxqt-themes.sh --all` |

All theme scripts support: `--all`, `--list`, `--dry-run`, and specific names.

---

## Utility Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| conky.sh | Install Conky configs | `bash conky.sh` |
| neofetch.sh | Install system fetch | `bash neofetch.sh` |
| emacs.sh | Install Emacs + config | `bash emacs.sh --install` |
| fix-gtk-window-controls.sh | Fix GTK button layout | `bash fix-gtk-window-controls.sh` |
| enable-natural-scrollong.sh | Enable natural scroll | `bash enable-natural-scrollong.sh` |
| enable-touchpad-tap.sh | Enable tap-to-click | `bash enable-touchpad-tap.sh` |
| set_default_wallpaper.sh | Set desktop wallpaper | `bash set_default_wallpaper.sh` |
| update_sddm_wallpaper.sh | Update SDDM background | `bash update_sddm_wallpaper.sh` |
| update_lxqt_panel.sh | Rebuild panel from source | `bash update_lxqt_panel.sh` |

---

## Root Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| install.sh | One-command installer | `bash install.sh --full` |
| archive.sh | Create distributable archive | `bash archive.sh` |
| fix-ntfs.sh | NTFS partition mounter | `bash fix-ntfs.sh` |
| fix-labwc-cursor-selection.sh | Fix labwc cursor config | `bash fix-labwc-cursor-selection.sh` |
| fix-clipboard-wayland.sh | Fix clipboard setup | `bash fix-clipboard-wayland.sh` |
| remove-gnome-keep-mutter-gtk.sh | Remove GNOME, keep Mutter | `bash remove-gnome-keep-mutter-gtk.sh` |
| remove-plasma-keep-kwin-sddm.sh | Remove Plasma, keep KWin | `bash remove-plasma-keep-kwin-sddm.sh` |
