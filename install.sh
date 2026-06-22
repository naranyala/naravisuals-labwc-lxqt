#!/bin/bash
# Naravisuals Desktop Suite — One-Command Installer
# Installs everything: dotfiles, features, themes, tools
#
# Usage:
#   bash install.sh                  # Interactive
#   bash install.sh --full           # Everything
#   bash install.sh --minimal        # Core configs only
#   bash install.sh --select         # Choose modules
#   bash install.sh --dry-run        # Preview only

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/lib.sh"

MODE="interactive"
DRY_RUN=false

for arg in "$@"; do
    case "$arg" in
        --full|-a)     MODE="full" ;;
        --minimal|-m)  MODE="minimal" ;;
        --select|-s)   MODE="select" ;;
        --dry-run|-n)  DRY_RUN=true ;;
        --help|-h)
            printf "Naravisuals Desktop Suite Installer\n\n"
            printf "Usage: bash install.sh [options]\n\n"
            printf "Options:\n"
            printf "  --full, -a      Install everything (configs + all features + themes)\n"
            printf "  --minimal, -m   Install core configs only\n"
            printf "  --select, -s    Choose which modules to install\n"
            printf "  --dry-run, -n   Preview without making changes\n"
            printf "  --help, -h      Show this help\n"
            exit 0
            ;;
    esac
done

print_header "Naravisuals Desktop Suite Installer"

# --- Step 1: Build dotfiles installer ---
log_step "Building dotfiles installer"
DOTFILES_BIN="$SCRIPT_DIR/cmd/dotfiles-manager/lxqt-dotfiles"

if [ ! -f "$DOTFILES_BIN" ] || [ "$MODE" = "full" ]; then
    if [ "$DRY_RUN" = true ]; then
        log_dim "[DRY-RUN] Would build: c++ -std=c++17 cmd/dotfiles-manager/build.cpp -o cmd/dotfiles-manager/lxqt-dotfiles"
    else
        cd "$SCRIPT_DIR/cmd/dotfiles-manager"
        c++ -std=c++17 build.cpp -o lxqt-dotfiles 2>/dev/null && {
            log_ok "Dotfiles installer built"
        } || {
            log_warn "Could not build dotfiles installer (missing g++?)"
            log_info "Install: sudo apt install g++ or sudo dnf install gcc-c++"
        }
        cd "$SCRIPT_DIR"
    fi
else
    log_ok "Dotfiles installer already built"
fi

# --- Step 2: Install dotfiles ---
log_step "Installing dotfiles to ~/.config"
if [ -f "$DOTFILES_BIN" ]; then
    if [ "$DRY_RUN" = true ]; then
        log_dim "[DRY-RUN] Would run: $DOTFILES_BIN install"
    else
        "$DOTFILES_BIN" install
    fi
else
    log_warn "Dotfiles installer not available, copying manually..."
    if [ "$DRY_RUN" != true ]; then
        # Fallback: manual copy
        for f in $(grep '"configs/' "$SCRIPT_DIR/cmd/dotfiles-manager/build.cpp" | sed 's/.*"configs\///' | sed 's/".*//' ); do
            src="$SCRIPT_DIR/configs/dotfiles/$f"
            dst="$HOME/.config/$(dirname "$f" | sed 's|^lxqt/||;s|^labwc/||;s|^gtk-3.0/||;s|^gtk-4.0/||')"
            mkdir -p "$HOME/.config/$(dirname "$f")"
            [ -f "$src" ] && cp "$src" "$HOME/.config/$f" 2>/dev/null
        done
        log_ok "Dotfiles copied (manual fallback)"
    fi
fi

# --- Step 3: Install system files (requires sudo) ---
log_step "Installing system files"
if [ "$DRY_RUN" != true ]; then
    if [ -f "$SCRIPT_DIR/configs/dotfiles/wayland-sessions/lxqt-labwc.desktop" ]; then
        sudo cp "$SCRIPT_DIR/configs/dotfiles/wayland-sessions/lxqt-labwc.desktop" \
            /usr/share/wayland-sessions/ 2>/dev/null && \
            log_ok "Installed wayland session" || log_warn "Could not install wayland session"
    fi
    if [ -f "$SCRIPT_DIR/configs/dotfiles/sddm/lxqt-labwc.conf" ]; then
        sudo mkdir -p /etc/sddm.conf.d/ 2>/dev/null
        sudo cp "$SCRIPT_DIR/configs/dotfiles/sddm/lxqt-labwc.conf" \
            /etc/sddm.conf.d/ 2>/dev/null && \
            log_ok "Installed SDDM config" || log_warn "Could not install SDDM config"
    fi
fi

# --- Step 4: Feature modules ---
if [ "$MODE" = "minimal" ]; then
    log_info "Minimal mode: skipping feature modules"
elif [ "$MODE" = "full" ] || [ "$MODE" = "select" ]; then
    log_step "Installing feature modules"
    if [ "$DRY_RUN" = true ]; then
        log_dim "[DRY-RUN] Would run feature modules"
    else
        bash "$SCRIPT_DIR/scripts/install-all.sh" $([ "$MODE" = "select" ] && echo "--select" || echo "--all")
    fi
fi

# --- Step 5: Build Control Center (optional) ---
if [ "$MODE" = "full" ]; then
    log_step "Building Control Center"
    if [ "$DRY_RUN" = true ]; then
        log_dim "[DRY-RUN] Would build Control Center"
    else
        if cmd_exists cmake && cmd_exists ninja; then
            cd "$SCRIPT_DIR/apps/control-center"
            mkdir -p build && cd build
            cmake -GNinja .. 2>/dev/null && ninja 2>/dev/null && {
                log_ok "Control Center built"
            } || {
                log_warn "Control Center build failed (Qt6 dev packages needed)"
            }
            cd "$SCRIPT_DIR"
        else
            log_warn "cmake/ninja not found, skipping Control Center build"
        fi
    fi
fi

# --- Summary ---
print_summary "Installation Complete" "ok"
printf "\n${BOLD}What was installed:${RST}\n"
printf "  - Dotfiles to ~/.config/\n"
printf "  - System files (SDDM, wayland session)\n"
if [ "$MODE" = "full" ]; then
    printf "  - Feature modules (clipboard, OSD, launcher, portals)\n"
    printf "  - Themes, icons, cursors, fonts, wallpapers\n"
    printf "  - Control Center binary\n"
fi
printf "\n${BOLD}Next steps:${RST}\n"
printf "  1. Log out and select 'LXQt (labwc)' from SDDM\n"
printf "  2. Run: ${GREEN}bash scripts/install-all.sh --select${RST} for additional themes\n"
printf "  3. Build Control Center: ${GREEN}cd apps/control-center/build && ./naravisuals-control-center${RST}\n"
