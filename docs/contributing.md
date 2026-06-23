# Contributing Guide

How to add features, scripts, configs, and documentation.

## Project Structure

```
naravisuals-labwc-lxqt/
  apps/           Qt6 GUIs (CMake + Ninja)
  cmd/            C++ deployment tools
  configs/        Static configuration files
  scripts/        Bash automation
  assets/         Media and resources
  docs/           Documentation
  install.sh      One-command installer
  archive.sh      Archive builder
  PKGBUILD        Arch AUR package
```

## Adding a New Script

### Location

Place the script in the correct directory:

| Type | Location | Example |
|------|----------|---------|
| Feature installer | `scripts/features/` | `install-clipboard.sh` |
| Theme installer | `scripts/` | `themes.sh` |
| Build script | `scripts/build/` | `build_latest_lxqt_desktop.sh` |
| Utility | `scripts/` | `fix-gtk-window-controls.sh` |

### Required Structure

Every feature script must:

1. Source `lib.sh` for shared utilities
2. Use `print_header` and `print_summary` for consistent output
3. Use `pkg_install` or `pkg_install_fallback` for package installation
4. Support `--help`, `--dry-run`, and `--all` flags
5. Use `log_info`, `log_ok`, `log_warn`, `log_error` for output

```bash
#!/bin/bash
# Script description
#
# Usage:
#   bash script.sh              # Interactive
#   bash script.sh --all        # Non-interactive
#   bash script.sh --dry-run    # Preview

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

DRY_RUN=false
FORCE=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        --all|-a) FORCE=true ;;
        --help|-h)
            printf "Usage: bash script.sh [options]\n"
            exit 0
            ;;
    esac
done

print_header "Script Name"

# ... implementation ...

print_summary "Script Name" "ok" "Optional detail message"
```

### Cross-Distro Support

Use `lib.sh` helpers for cross-distro compatibility:

```bash
# Detect package manager
PM="$(detect_pm)"

case "$PM" in
    apt)     sudo apt-get install -y package ;;
    dnf)     sudo dnf install -y package ;;
    pacman)  sudo pacman -S --noconfirm package ;;
    *)       log_warn "Unknown package manager"; return 1 ;;
esac

# Or use the helper
pkg_install "package-name"

# Check if installed
is_installed "package-name"

# Check if available in repos
pkg_available "package-name"
```

## Adding a New Config File

### Location

Place the config in the correct subdirectory:

| Component | Location | Destination |
|-----------|----------|-------------|
| Labwc | `configs/dotfiles/labwc/` | `~/.config/labwc/` |
| LXQt | `configs/dotfiles/lxqt/` | `~/.config/lxqt/` |
| GTK | `configs/dotfiles/gtk-*/` | `~/.config/gtk-*/` |
| Tool-specific | `configs/dotfiles/<tool>/` | `~/.config/<tool>/` |

### Required Steps

1. Create the config file
2. Add it to `cmd/dotfiles-manager/build.cpp` manifest
3. Add it to the archive.sh include list if needed
4. Document it in `docs/configuration.md`

**Manifest entry format:**

```cpp
{"configs/dotfiles/tool/config", ".config/tool/config"},
```

## Adding a New GUI Page

### Location

Add a new page class to `apps/control-center/main.cpp`.

### Required Steps

1. Create a new `QWidget` subclass
2. Add it to the `QStackedWidget`
3. Add a sidebar entry
4. Use `runScript()` or `runTerminalScript()` for button actions

```cpp
class MyNewPage : public QWidget {
public:
    MyNewPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);

        auto *group = new QGroupBox("My Feature");
        auto *gLayout = new QVBoxLayout(group);
        gLayout->setSpacing(10);

        auto *btn = new QPushButton("Do Something");
        connect(btn, &QPushButton::clicked, [](){ runTerminalScript("my-script.sh"); });
        gLayout->addWidget(btn);

        layout->addWidget(group);
        layout->addStretch();
    }
};
```

Then in `ControlCenterWindow`:
```cpp
sidebar->addItem("My Feature");
pages->addWidget(new MyNewPage());
```

## Adding a New Compositor Profile

### Location

Create `configs/compositors/<name>/` with the compositor's config file.

### Required Steps

1. Create the config directory and file
2. Add it to `scripts/features/install-compositor.sh` case statement
3. Add it to `cmd/dotfiles-manager/build.cpp` manifest
4. Document it in `docs/compositors.md`

## Documentation

### Writing Docs

All documentation goes in `docs/`. Use clear headings, tables, and code blocks.

**Required docs for new features:**
- Add to `docs/scripts.md` if it is a script
- Add to `docs/configuration.md` if it is a config
- Add to `docs/applications.md` if it is a GUI
- Update `docs/README.md` index if creating a new doc file

### Code Comments

Do not add comments to code unless requested. Code should be self-documenting through clear naming.

## Testing

### Before Committing

```bash
# Check syntax of all modified scripts
bash -n scripts/lib.sh
bash -n scripts/my-script.sh

# Test in dry-run mode
bash scripts/my-script.sh --dry-run

# Test the Control Center builds
cd apps/control-center/build && ninja

# Test the archive includes new files
bash archive.sh --list

# Run pre-flight checks
bash install.sh --check
```

### What to Verify

- Scripts work on both apt and dnf systems (if possible)
- New configs are in the dotfiles manifest
- New scripts are in the install-all.sh modules list
- Archive includes all new files
- Documentation is updated

## Commit Messages

Use clear, descriptive commit messages:

```
Add clipboard management support

- Add cliphist config to configs/dotfiles/cliphist/
- Add install-clipboard.sh feature script
- Update autostart with clipboard history daemon
- Add to dotfiles installer manifest (33 entries)
- Add to install-all.sh orchestrator (20 modules)
```

## Pull Requests

1. Fork the repository
2. Create a feature branch
3. Make changes following the guidelines above
4. Test on at least one distro
5. Update documentation
6. Submit a pull request with a clear description
