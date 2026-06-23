# Installation Guide

Step-by-step installation for all supported distributions.

## Requirements

| Component | Minimum Version |
|-----------|-----------------|
| C++ compiler | GCC 9+ or Clang 10+ (C++17) |
| CMake | 3.16+ |
| Ninja | 1.10+ |
| Qt6 | 6.5+ |
| Labwc | 0.7.1+ |
| LXQt | 2.1+ |

## Quick Install

```bash
git clone https://github.com/naranyala/naravisuals-labwc-lxqt.git
cd naravisuals-labwc-lxqt
bash install.sh
```

## Install Modes

### Full Install (recommended)

Installs everything: dotfiles, feature modules, themes, icons, fonts, wallpapers, and builds the Control Center.

```bash
bash install.sh --full
```

### Minimal Install

Installs only core dotfiles. No network required, no feature modules, no theme downloads.

```bash
bash install.sh --minimal
```

### Selective Install

Interactive menu to choose which modules to install.

```bash
bash install.sh --select
```

### Dry Run

Preview what would be installed without making changes.

```bash
bash install.sh --dry-run --full
```

### Pre-flight Check

Validate your environment before installing.

```bash
bash install.sh --check
```

## Step-by-Step Manual Install

### Step 1: Build LXQt from Source (optional)

If your distro ships an outdated LXQt:

```bash
sudo ./scripts/build/build_latest_lxqt_desktop.sh
```

### Step 2: Fedora/RHEL Pre-setup (if applicable)

```bash
bash scripts/features/setup-fedora-repos.sh
```

### Step 3: Install Dotfiles

```bash
cd cmd/dotfiles-manager
c++ -std=c++17 build.cpp -o lxqt-dotfiles
./lxqt-dotfiles install
```

### Step 4: Install System Files

```bash
sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/
sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/
```

### Step 5: Install Feature Modules

```bash
bash scripts/install-all.sh --select
```

### Step 6: Build Control Center (optional)

```bash
cd apps/control-center
mkdir -p build && cd build
cmake -GNinja .. && ninja
./nv-control-center
```

## Post-Install

1. Log out of your current session
2. Select "LXQt (labwc)" from SDDM
3. Run `bash scripts/install-all.sh --select` for additional themes
4. Run `bash scripts/reset-panel.sh` if the panel needs resetting

## Arch Linux (AUR)

```bash
makepkg -si
nv-dotfiles install
```

## Recovery

If installation fails partway through:

```bash
bash install.sh --recover
```

This skips completed steps and re-runs failures.

View the installation log:

```bash
cat ~/.local/share/naravisuals-install.log
```
