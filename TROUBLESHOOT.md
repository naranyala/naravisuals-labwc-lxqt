# Troubleshooting Guide

This document covers common installation and runtime issues, their causes, and solutions.

---

## Installation Errors

### "c++: command not found" or "g++: command not found"

The dotfiles installer requires a C++17 compiler.

```bash
# Debian/Ubuntu
sudo apt install g++

# Fedora/RHEL
sudo dnf install gcc-c++

# Arch
sudo pacman -S gcc
```

### "cmake: command not found" or "ninja: command not found"

The Control Center and SDDM GUI require CMake and Ninja to build.

```bash
# Debian/Ubuntu
sudo apt install cmake ninja-build

# Fedora/RHEL
sudo dnf install cmake ninja-build

# Arch
sudo pacman -S cmake ninja
```

### "No network connectivity" during install

The installer needs network access to download themes, icons, fonts, and wallpapers.

```bash
# Test connectivity
ping -c3 github.com

# If behind a proxy, set these before running install.sh
export http_proxy=http://proxy:port
export https_proxy=http://proxy:port

# Or install in minimal mode (no downloads)
bash install.sh --minimal
```

### "Insufficient disk space"

Full installation requires approximately 2GB of free space.

```bash
# Check available space
df -h ~

# Clean package cache
sudo apt clean          # Debian/Ubuntu
sudo dnf clean all      # Fedora/RHEL
sudo pacman -Scc        # Arch
```

### "sudo: no password supplied" or permission denied

The installer needs sudo for system file installation.

```bash
# Verify sudo access
sudo -v

# If locked out, run minimal mode (no sudo needed)
bash install.sh --minimal
```

### Dotfiles installer build fails with "filesystem: No such file or directory"

Your compiler does not support C++17 `<filesystem>`.

```bash
# Check compiler version
g++ --version

# Need GCC 9+ or Clang 10+
# On older Ubuntu, install a newer GCC
sudo apt install gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

---

## Package Installation Errors

### "EPEL not found" on Fedora/RHEL

```bash
# Fedora does not need EPEL — skip this
# On RHEL/CentOS/Rocky/Alma:
sudo dnf install -y epel-release

# If epel-release is not available, install manually
sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-$(rpm -E %rhel).noarch.rpm
```

### "Copr repo not found" or "Failed to enable Copr"

```bash
# Install dnf-plugins-core first
sudo dnf install -y dnf-plugins-core

# Then retry
bash scripts/features/setup-fedora-repos.sh
```

### "wob not found" and cargo build fails

wob may not be in your distro repos. Build from source requires Rust.

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"

# Then retry
bash scripts/features/install-osd.sh
```

### "cliphist not found" on Debian/Ubuntu

cliphist is not in Debian/Ubuntu repos. Build from source.

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"

# Install cliphist
cargo install cliphist

# Or use CopyQ instead (available in repos)
sudo apt install copyq
```

### "rofi" installs but Wayland mode does not work

The standard rofi package does not support Wayland natively.

```bash
# On Arch, install the Wayland fork
sudo pacman -S rofi-wayland

# On Debian/Ubuntu, build from source
sudo apt install build-essential cmake meson libpango1g-dev \
    libglib2.0-dev libpangocairo-dev libgdk-pixbuf-2.0-dev \
    libwayland-dev wayland-protocols libxkbcommon-dev
git clone https://github.com/lbonn/rofi.git
cd rofi && mkdir build && cd build
cmake -GNinja .. && ninja
sudo ninja install
```

### "kvantum not found" in package repos

```bash
# On Debian/Ubuntu
sudo apt install kvantum

# On Fedora, try Copr
sudo dnf copr enable -y tsujan/kvantum
sudo dnf install -y kvantum

# On Arch
sudo pacman -S kvantum
```

---

## Runtime Errors

### LXQt session does not start (black screen)

```bash
# Switch to TTY (Ctrl+Alt+F2), log in, then:

# Check session.conf
cat ~/.config/lxqt/session.conf | grep window_manager

# Reset to labwc
sed -i 's/^window_manager=.*/window_manager=labwc/' ~/.config/lxqt/session.conf

# Verify labwc is installed
which labwc
labwc --version

# Try starting manually
labwc &
```

### Panel does not show window buttons

The panel needs the compositor's wlr-foreign-toplevel protocol. Labwc supports this natively.

```bash
# If using KWin Wayland, set this before launching the panel
export LXQT_PANEL_WAYLAND_BACKEND=KWindowSystem

# If panel is corrupted, reset to stock
bash scripts/reset-panel.sh
labwc -r
```

### Qt applications look wrong / theme not applying

```bash
# Check platform theme
echo $QT_QPA_PLATFORMTHEME

# Should be qt6ct (not qt5ct for Qt6 apps)
export QT_QPA_PLATFORMTHEME=qt6ct

# Add to ~/.config/labwc/environment if missing
grep -q "QT_QPA_PLATFORMTHEME" ~/.config/labwc/environment || \
    echo "QT_QPA_PLATFORMTHEME=qt6ct" >> ~/.config/labwc/environment

# If using Kvantum
export QT_STYLE_OVERRIDE=kvantum

# Run qt6ct to configure
qt6ct
```

### No system tray icons

```bash
# Check if Status Notifier plugin is in the panel
grep "statusnotifier" ~/.config/lxqt/panel.conf

# If missing, add it to the plugins list
# Edit panel.conf and add "statusnotifier" to the plugins= line

# Restart panel
pkill -x lxqt-panel
lxqt-panel &
```

### Clipboard history not working

```bash
# Check if wl-paste and cliphist are running
pgrep -a wl-paste
pgrep -a cliphist

# If not running, start manually
wl-paste --type text --watch cliphist store &
wl-paste --type image --watch cliphist store &

# Test clipboard
echo "test" | wl-copy
wl-paste
cliphist list
```

### Screen sharing does not work

```bash
# Check portal stack
systemctl --user status xdg-desktop-portal
systemctl --user status xdg-desktop-portal-wlr

# If not running, start them
/usr/libexec/xdg-desktop-portal &
/usr/libexec/xdg-desktop-portal-wlr &

# Check PipeWire
systemctl --user status pipewire
systemctl --user status wireplumber

# If PipeWire is not running, install and start it
sudo apt install pipewire wireplumber   # Debian/Ubuntu
sudo dnf install pipewire wireplumber   # Fedora/RHEL
systemctl --user --now enable pipewire wireplumber
```

### OSD (wob) does not show volume/brightness bar

```bash
# Check if wob is installed
which wob

# Check if the FIFO exists
ls -la /tmp/wob_fifo

# If not, create it and restart
mkfifo /tmp/wob_fifo 2>/dev/null || true
tail -f /tmp/wob_fifo | wob &

# Test manually
echo "50%" > /tmp/wob_fifo
```

### Wallpaper does not appear

```bash
# Check if swaybg is running
pgrep -a swaybg

# If not, start it manually
WALLPAPER=$(cat ~/.config/labwc/wallpaper 2>/dev/null || echo "/usr/share/backgrounds/warty-final-ubuntu.png")
swaybg -i "$WALLPAPER" -m fill &

# Check wallpaper path exists
ls -la "$WALLPAPER"
```

### GTK applications have wrong cursor

```bash
# Check current cursor settings
gsettings get org.gnome.desktop.interface cursor-theme
gsettings get org.gnome.desktop.interface cursor-size

# Set manually
gsettings set org.gnome.desktop.interface cursor-theme 'breeze_cursors'
gsettings set org.gnome.desktop.interface cursor-size 24

# These should match ~/.config/lxqt/session.conf
grep cursor ~/.config/lxqt/session.conf
```

### SDDM does not show the Wayland session

```bash
# Check session file exists
ls -la /usr/share/wayland-sessions/lxqt-labwc.desktop

# If missing, copy it
sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/

# Check SDDM config
ls -la /etc/sddm.conf.d/lxqt-labwc.conf

# If missing, copy it
sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/
```

---

## Recovery Procedures

### Re-run failed installation steps

```bash
# Check what was completed
cat /tmp/naravisuals-install-steps

# Re-run install (it skips completed steps)
bash install.sh --full
```

### Reset everything to defaults

```bash
# Backup current configs
mv ~/.config/labwc ~/.config/labwc.bak
mv ~/.config/lxqt ~/.config/lxqt.bak

# Re-install dotfiles
cd cmd/dotfiles-manager
./lxqt-dotfiles install

# Reconfigure
labwc -r
```

### Restore panel from backup

```bash
# List backups
ls -la ~/.config/lxqt/backups/

# Restore most recent
bash scripts/reset-panel.sh --restore
labwc -r
```

### Fix broken compositor config

```bash
# Reset labwc config to defaults
rm ~/.config/labwc/rc.xml
cp /usr/share/doc/labwc/rc.xml ~/.config/labwc/ 2>/dev/null || \
cp /usr/share/doc/labwc/rc.xml.all ~/.config/labwc/rc.xml 2>/dev/null

# Reconfigure
labwc -r
```

---

## Log Files

The installer logs all operations to:

```
~/.local/share/naravisuals-install.log
```

View the log:

```bash
cat ~/.local/share/naravisuals-install.log
```

Per-script logs are written to:

```
/tmp/naravisuals-install.log
```

---

## Getting Help

1. Run pre-flight checks: `bash install.sh --check`
2. Check the install log: `cat ~/.local/share/naravisuals-install.log`
3. Re-run in dry-run mode: `bash install.sh --dry-run --full`
4. Open an issue at: https://github.com/naranyala/naravisuals-labwc-lxqt/issues
