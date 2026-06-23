#!/bin/bash
# Naravisuals — Canonical Dotfiles Installer
# ============================================
# Single source of truth for deploying configs to ~/.config.
# Supports: --dry-run, --force, --backup, --restore, --validate
#
# This script:
#   1. Backs up existing configs before overwriting
#   2. Deploys all dotfiles from configs/dotfiles/
#   3. Copies fonts/wallpapers to XDG dirs
#   4. Rebuilds font cache
#   5. Validates the result
#
# Usage:
#   bash install.sh                   # Install (skip existing)
#   bash install.sh --force           # Overwrite existing
#   bash install.sh --dry-run         # Preview only
#   bash install.sh --restore         # Restore from backup
#   bash install.sh --validate        # Validate only
#   bash install.sh --force --backup  # Overwrite with backup

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOTFILES_DIR="$PROJECT_ROOT/configs/dotfiles"

# ---- Flags ----
DRY_RUN=false
FORCE=false
DO_BACKUP=true
DO_RESTORE=false
DO_VALIDATE=false

# ---- Logging ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
DIM='\033[2m'
RST='\033[0m'

log()    { printf "${BLUE}::${RST} %s\n" "$*"; }
log_ok() { printf "${GREEN}  ${RST} %s\n" "$*"; }
log_warn(){ printf "${YELLOW}  !${RST} %s\n" "$*" >&2; }
log_err() { printf "${RED}  ${RST} %s\n" "$*" >&2; }
log_dim() { printf "${DIM}    %s${RST}\n" "$*"; }

# ---- Parse args ----
for arg in "$@"; do
    case "$arg" in
        --dry-run)   DRY_RUN=true ;;
        --force|-f)  FORCE=true ;;
        --no-backup) DO_BACKUP=false ;;
        --restore)   DO_RESTORE=true ;;
        --validate)  DO_VALIDATE=true ;;
        --help|-h)
            cat <<EOF
Naravisuals Dotfiles Installer

Usage: bash install.sh [options]

Options:
  --force, -f      Overwrite existing files (backs up first)
  --dry-run        Show what would be installed without writing
  --no-backup      Skip backup before overwriting
  --restore        Restore configs from last backup
  --validate       Run validation checks only
  --help, -h       Show this help

This script deploys configs from configs/dotfiles/ to ~/.config/.
Backups are stored in ~/.config/backups/naravisuals/<timestamp>/
EOF
            exit 0
            ;;
        *)
            log_err "Unknown option: $arg (use --help for usage)"
            exit 1
            ;;
    esac
done

# ---- Validate mode ----
if [ "$DO_VALIDATE" = true ]; then
    if [ -f "$PROJECT_ROOT/validate.sh" ]; then
        exec bash "$PROJECT_ROOT/validate.sh"
    else
        log_err "validate.sh not found"
        exit 1
    fi
fi

# ---- Verify source directory ----
if [ ! -d "$DOTFILES_DIR" ]; then
    log_err "Source directory not found: $DOTFILES_DIR"
    log_err "Are you running this from the project root?"
    exit 1
fi

# ---- Backup system ----
BACKUP_DIR="$HOME/.config/backups/naravisuals/$(date +%Y%m%d_%H%M%S)"
backup_file() {
    local dst="$1"
    if [ -f "$dst" ] && [ "$DO_BACKUP" = true ]; then
        local rel="${dst#$HOME/}"
        local backup_path="$BACKUP_DIR/$rel"
        mkdir -p "$(dirname "$backup_path")"
        cp "$dst" "$backup_path"
    fi
}

# ---- Restore mode ----
if [ "$DO_RESTORE" = true ]; then
    LATEST_BACKUP=$(find "$HOME/.config/backups/naravisuals/" -maxdepth 1 -type d 2>/dev/null | sort -r | head -1)
    if [ -z "$LATEST_BACKUP" ] || [ ! -d "$LATEST_BACKUP" ]; then
        log_err "No backups found in ~/.config/backups/naravisuals/"
        exit 1
    fi
    log "Restoring from: $LATEST_BACKUP"
    find "$LATEST_BACKUP" -type f | while read -r backup_file; do
        rel="${backup_file#$LATEST_BACKUP/}"
        dst="$HOME/$rel"
        mkdir -p "$(dirname "$dst")"
        cp "$backup_file" "$dst"
        log_ok "Restored: $rel"
    done
    log "Restore complete"
    exit 0
fi

# ---- Config file manifest ----
# Format: "SOURCE_PATH|DEST_PATH|PERMISSIONS"
# SOURCE_PATH is relative to DOTFILES_DIR
# DEST_PATH is relative to $HOME
declare -a MANIFEST=(
    "lxqt/session.conf|config/lxqt/session.conf|644"
    "lxqt/panel.conf|config/lxqt/panel.conf|644"
    "lxqt/lxqt.conf|config/lxqt/lxqt.conf|644"
    "lxqt/lxqt-config.conf|config/lxqt/lxqt-config.conf|644"
    "lxqt/lxqt-powermanagement.conf|config/lxqt/lxqt-powermanagement.conf|644"
    "lxqt/lxqt-runner.conf|config/lxqt/lxqt-runner.conf|644"
    "lxqt/lxqt-notificationd.conf|config/lxqt/lxqt-notificationd.conf|644"
    "lxqt/globalkeyshortcuts.conf|config/lxqt/globalkeyshortcuts.conf|644"
    "lxqt/lxqt-panel.qss|config/lxqt/lxqt-panel.qss|644"
    "lxqt/panel-stock.conf|config/lxqt/panel-stock.conf|644"
    "labwc/rc.xml|config/labwc/rc.xml|644"
    "labwc/menu.xml|config/labwc/menu.xml|644"
    "labwc/autostart|config/labwc/autostart|755"
    "labwc/environment|config/labwc/environment|644"
    "labwc/themerc|config/labwc/themerc-override|644"
    "labwc/shutdown|config/labwc/shutdown|755"
    "gtk-3.0/settings.ini|config/gtk-3.0/settings.ini|644"
    "gtk-4.0/settings.ini|config/gtk-4.0/settings.ini|644"
    "qt6ct/qt6ct.conf|config/qt6ct/qt6ct.conf|644"
    "pcmanfm-qt/lxqt/settings.conf|config/pcmanfm-qt/lxqt/settings.conf|644"
    "qterminal.org/qterminal.ini|config/qterminal.org/qterminal.ini|644"
    "user-dirs.dirs|config/user-dirs.dirs|644"
    "fontconfig/fonts.conf|config/fontconfig/fonts.conf|644"
    "kanshi/config|config/kanshi/config|644"
    "swaylock/config|config/swaylock/config|644"
    "dunst/dunstrc|config/dunst/dunstrc|644"
    "cliphist/config|config/cliphist/config|644"
    "wob/config|config/wob/config|644"
    "rofi/config.rasi|config/rofi/config.rasi|644"
    "xdg-desktop-portal-wlr/config|config/xdg-desktop-portal-wlr/config|644"
    "emacs/init.el|config/emacs/init.el|644"
)

# Directories to copy recursively (relative to DOTFILES_DIR)
declare -a DIR_MANIFEST=(
    "fonts|local/share/fonts"
    "wallpapers|local/share/wallpapers"
)

# ---- Deploy config files ----
INSTALLED=0
SKIPPED=0
BACKED_UP=0
FAILED=0

printf "\n${BOLD}Deploying dotfiles...${RST}\n\n"

for entry in "${MANIFEST[@]}"; do
    IFS='|' read -r src rel_dst perm <<< "$entry"
    src_path="$DOTFILES_DIR/$src"
    dst_path="$HOME/$rel_dst"

    # Check source exists
    if [ ! -f "$src_path" ]; then
        log_warn "Source missing: $src"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Dry run
    if [ "$DRY_RUN" = true ]; then
        if [ -f "$dst_path" ]; then
            log_dim "would update: $rel_dst"
        else
            log_dim "would create: $rel_dst"
        fi
        INSTALLED=$((INSTALLED + 1))
        continue
    fi

    # Skip if exists and not forcing
    if [ -f "$dst_path" ] && [ "$FORCE" != true ]; then
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # Backup if overwriting
    if [ -f "$dst_path" ] && [ "$FORCE" = true ]; then
        backup_file "$dst_path"
        BACKED_UP=$((BACKED_UP + 1))
    fi

    # Install
    mkdir -p "$(dirname "$dst_path")"
    cp "$src_path" "$dst_path"
    chmod "$perm" "$dst_path"
    INSTALLED=$((INSTALLED + 1))
done

# ---- Deploy directories ----
for entry in "${DIR_MANIFEST[@]}"; do
    IFS='|' read -r src rel_dst <<< "$entry"
    src_path="$DOTFILES_DIR/$src"
    dst_path="$HOME/$rel_dst"

    if [ ! -d "$src_path" ]; then
        continue
    fi

    if [ "$DRY_RUN" = true ]; then
        log_dim "would create dir: $rel_dst"
        continue
    fi

    mkdir -p "$dst_path"
    # Copy with hidden files support
    cp -r "$src_path"/. "$dst_path"/ 2>/dev/null || true
    log_ok "dir: $rel_dst"
done

# ---- Rebuild font cache ----
if [ "$DRY_RUN" != true ] && command -v fc-cache &>/dev/null; then
    if [ -d "$HOME/.local/share/fonts" ]; then
        log "Rebuilding font cache..."
        fc-cache -f 2>/dev/null && log_ok "Font cache rebuilt" || log_warn "fc-cache failed"
    fi
fi

# ---- Summary ----
printf "\n"
if [ "$DRY_RUN" = true ]; then
    printf "${BOLD}Dry run complete:${RST} %d files would be installed\n" "$INSTALLED"
else
    printf "${BOLD}Install complete:${RST} %d installed, %d skipped, %d backed up" \
        "$INSTALLED" "$SKIPPED" "$BACKED_UP"
    [ "$FAILED" -gt 0 ] && printf ", ${RED}%d failed${RST}" "$FAILED"
    printf "\n"
fi

if [ "$BACKED_UP" -gt 0 ] && [ "$DRY_RUN" != true ]; then
    printf "\n${DIM}Backups saved to: %s${RST}\n" "$BACKUP_DIR"
    printf "${DIM}Restore with: bash %s --restore${RST}\n" "$0"
fi

# ---- Validate after install ----
if [ "$DRY_RUN" != true ] && [ "$INSTALLED" -gt 0 ]; then
    if [ -f "$PROJECT_ROOT/validate.sh" ]; then
        printf "\n${BOLD}Running validation...${RST}\n"
        if bash "$PROJECT_ROOT/validate.sh" --quiet 2>/dev/null; then
            printf "${GREEN}All checks passed.${RST}\n"
        else
            printf "${YELLOW}Some checks failed. Run: bash validate.sh${RST}\n"
        fi
    fi
fi

[ "$FAILED" -eq 0 ] || exit 1
