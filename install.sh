#!/bin/bash
# Naravisuals Desktop Suite — One-Command Installer
# ====================================================
# Thin orchestrator that delegates to specialized installers.
#
# Usage:
#   bash install.sh                  # Interactive
#   bash install.sh --full           # Everything
#   bash install.sh --minimal        # Core configs only
#   bash install.sh --select         # Choose modules
#   bash install.sh --dry-run        # Preview only
#   bash install.sh --recover        # Fix partial install
#   bash install.sh --check          # Pre-flight checks only

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
        --recover|-r)  MODE="recover" ;;
        --check|-c)    MODE="check" ;;
        --help|-h)
            printf "Naravisuals Desktop Suite Installer\n\n"
            printf "Usage: bash install.sh [options]\n\n"
            printf "Options:\n"
            printf "  --full, -a      Install everything (configs + all features + themes)\n"
            printf "  --minimal, -m   Install core configs only\n"
            printf "  --select, -s    Choose which modules to install\n"
            printf "  --dry-run, -n   Preview without making changes\n"
            printf "  --recover, -r   Fix partial install (re-run failed steps)\n"
            printf "  --check, -c     Run pre-flight checks only\n"
            printf "  --help, -h      Show this help\n"
            exit 0
            ;;
    esac
done

# ---- Logging ----
INSTALL_LOG="${HOME}/.local/share/naravisuals-install.log"
mkdir -p "$(dirname "$INSTALL_LOG")" 2>/dev/null || true
LOG_FILE="$INSTALL_LOG"

echo "=== Naravisuals Install $(date) ===" >> "$LOG_FILE"
echo "Mode: $MODE" >> "$LOG_FILE"

# ---- Check mode ----
if [ "$MODE" = "check" ]; then
    check_prereqs "full"
    exit $?
fi

print_header "Naravisuals Desktop Suite Installer"

# ---- Pre-flight ----
if [ "$MODE" != "minimal" ]; then
    check_prereqs "$MODE" || {
        log_error "Pre-flight checks failed. Run with --check for details."
        log_info "Log saved to: $INSTALL_LOG"
        exit 1
    }
fi

# ---- Step tracking (persistent, not /tmp) ----
STEP_FILE="$SCRIPT_DIR/.install-state"
touch "$STEP_FILE"

step_done() {
    local step="$1"
    grep -q "^${step}$" "$STEP_FILE" 2>/dev/null
}

step_mark() {
    local step="$1"
    echo "$step" >> "$STEP_FILE"
}

step_clear() {
    local step="$1"
    sed -i "/^${step}$/d" "$STEP_FILE" 2>/dev/null || true
}

# ---- Step 1: Build dotfiles installer (optional, for speed) ----
step_name="build-dotfiles"
if ! step_done "$step_name"; then
    log_step "Building dotfiles installer"
    DOTFILES_BIN="$SCRIPT_DIR/cmd/dotfiles-manager/lxqt-dotfiles"

    if [ ! -f "$DOTFILES_BIN" ] || [ "$MODE" = "full" ]; then
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would build: c++ -std=c++17 build.cpp -o lxqt-dotfiles"
        else
            BUILD_DIR="$SCRIPT_DIR/cmd/dotfiles-manager"
            if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/build.cpp" ]; then
                log_warn "C++ build source not found, using shell installer"
            elif ! cmd_exists "c++" && ! cmd_exists "g++" && ! cmd_exists "clang++"; then
                log_warn "No C++ compiler found, using shell installer"
                log_info "Install: sudo apt install g++"
            else
                cd "$BUILD_DIR"
                if c++ -std=c++17 build.cpp -o lxqt-dotfiles 2>>"$LOG_FILE"; then
                    log_ok "Dotfiles installer built"
                    step_mark "$step_name"
                else
                    log_warn "Build failed, using shell installer"
                fi
                cd "$SCRIPT_DIR"
            fi
        fi
    else
        log_ok "Dotfiles installer already built"
        step_mark "$step_name"
    fi
fi

# ---- Step 2: Install dotfiles (canonical installer) ----
step_name="install-dotfiles"
if ! step_done "$step_name"; then
    log_step "Installing dotfiles to ~/.config"

    INSTALLER_ARGS=""
    [ "$DRY_RUN" = true ] && INSTALLER_ARGS="--dry-run"
    [ "$FORCE" = true ] && INSTALLER_ARGS="$INSTALLER_ARGS --force"

    # Prefer C++ binary if available, fall back to shell script
    if [ -f "${DOTFILES_BIN:-}" ]; then
        log "Using C++ installer"
        if "$DOTFILES_BIN" install 2>>"$LOG_FILE"; then
            log_ok "Dotfiles installed"
            step_mark "$step_name"
        else
            log_warn "C++ installer had issues, trying shell installer"
            step_clear "$step_name"
        fi
    fi

    # Shell installer (canonical — handles backup, validation, etc.)
    if ! step_done "$step_name"; then
        log "Using shell installer"
        if bash "$SCRIPT_DIR/scripts/build/install.sh" $INSTALLER_ARGS 2>>"$LOG_FILE"; then
            log_ok "Dotfiles installed"
            step_mark "$step_name"
        else
            log_error "Dotfile installation failed (check log)"
            log_info "Log: $INSTALL_LOG"
            log_info "Retry: bash install.sh --recover"
        fi
    fi
fi

# ---- Step 3: Install system files ----
step_name="install-system"
if ! step_done "$step_name"; then
    log_step "Installing system files"
    if [ "$DRY_RUN" != true ]; then
        sys_fail=0
        if [ -f "$SCRIPT_DIR/configs/dotfiles/wayland-sessions/lxqt-labwc.desktop" ]; then
            if sudo cp "$SCRIPT_DIR/configs/dotfiles/wayland-sessions/lxqt-labwc.desktop" \
                /usr/share/wayland-sessions/ 2>>"$LOG_FILE"; then
                log_ok "Installed wayland session"
            else
                log_warn "Could not install wayland session (sudo required)"
                sys_fail=$((sys_fail + 1))
            fi
        fi
        if [ -f "$SCRIPT_DIR/configs/dotfiles/sddm/lxqt-labwc.conf" ]; then
            if sudo mkdir -p /etc/sddm.conf.d/ 2>/dev/null && \
               sudo cp "$SCRIPT_DIR/configs/dotfiles/sddm/lxqt-labwc.conf" \
                /etc/sddm.conf.d/ 2>>"$LOG_FILE"; then
                log_ok "Installed SDDM config"
            else
                log_warn "Could not install SDDM config (sudo required)"
                sys_fail=$((sys_fail + 1))
            fi
        fi
        if [ "$sys_fail" -eq 0 ]; then
            step_mark "$step_name"
        fi
    fi
fi

# ---- Step 4: Feature modules ----
step_name="install-features"
if [ "$MODE" = "minimal" ]; then
    log_info "Minimal mode: skipping feature modules"
elif [ "$MODE" = "full" ] || [ "$MODE" = "select" ]; then
    if ! step_done "$step_name"; then
        log_step "Installing feature modules"
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would run feature modules"
        else
            select_flag=""
            run_flags=""
            [ "$MODE" = "select" ] && select_flag="--select"
            [ "$MODE" = "full" ] && run_flags="--all"
            if bash "$SCRIPT_DIR/scripts/install-all.sh" $select_flag $run_flags 2>>"$LOG_FILE"; then
                log_ok "Feature modules installed"
                step_mark "$step_name"
            else
                log_warn "Some feature modules had errors (check log)"
                log_info "Re-run with: bash install.sh --recover"
            fi
        fi
    fi
fi

# ---- Step 5: Build Control Center (optional) ----
step_name="build-control-center"
if [ "$MODE" = "full" ]; then
    if ! step_done "$step_name"; then
        log_step "Building Control Center"
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would build Control Center"
        elif [ ! -d "$SCRIPT_DIR/apps/control-center" ]; then
            log_warn "apps/control-center not found, skipping"
        elif ! cmd_exists cmake || ! cmd_exists ninja; then
            log_warn "cmake/ninja not found, skipping Control Center"
            log_info "Install: sudo apt install cmake ninja-build"
        else
            cd "$SCRIPT_DIR/apps/control-center"
            rm -rf build 2>/dev/null || true
            mkdir -p build && cd build
            if cmake -GNinja .. 2>>"$LOG_FILE" && ninja 2>>"$LOG_FILE"; then
                log_ok "Control Center built"
                step_mark "$step_name"
            else
                log_warn "Control Center build failed"
                log_info "Install Qt6 dev packages: sudo apt install qt6-base-dev"
            fi
            cd "$SCRIPT_DIR"
        fi
    fi
fi

# ---- Step 6: Run validation ----
log_step "Validating installation"
if [ -f "$SCRIPT_DIR/validate.sh" ]; then
    if bash "$SCRIPT_DIR/validate.sh" --quiet 2>/dev/null; then
        log_ok "Validation passed"
    else
        log_warn "Some validation checks failed"
        log_info "Run: bash validate.sh"
    fi
fi

# ---- Summary ----
print_summary "Installation Complete" "ok"

printf "\n${BOLD}Installation log:${RST} %s\n" "$INSTALL_LOG"
printf "\n${BOLD}What was installed:${RST}\n"
step_done "build-dotfiles"      && printf "  [done] Dotfiles installer built\n"   || printf "  [--] Dotfiles installer\n"
step_done "install-dotfiles"    && printf "  [done] Dotfiles to ~/.config/\n"    || printf "  [--] Dotfiles\n"
step_done "install-system"      && printf "  [done] System files\n"              || printf "  [--] System files\n"
step_done "install-features"    && printf "  [done] Feature modules\n"           || printf "  [--] Feature modules\n"
step_done "build-control-center" && printf "  [done] Control Center\n"           || printf "  [--] Control Center\n"

printf "\n${BOLD}Next steps:${RST}\n"
printf "  1. Log out and select 'LXQt (labwc)' from SDDM\n"
printf "  2. Run: ${GREEN}bash scripts/install-all.sh --select${RST} for additional themes\n"
printf "\n${BOLD}Troubleshooting:${RST}\n"
printf "  - View log: ${GREEN}cat %s${RST}\n" "$INSTALL_LOG"
printf "  - Validate: ${GREEN}bash validate.sh${RST}\n"
printf "  - Restore:  ${GREEN}bash scripts/build/install.sh --restore${RST}\n"
printf "  - Full guide: ${GREEN}cat TROUBLESHOOT.md${RST}\n"
