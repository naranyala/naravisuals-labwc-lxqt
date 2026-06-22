#!/bin/bash
# Reset LXQt Panel to Stock Configuration
# Backs up current panel.conf and replaces with the project default.
#
# Usage:
#   bash reset-panel.sh              # Interactive confirmation
#   bash reset-panel.sh --force      # Skip confirmation
#   bash reset-panel.sh --dry-run    # Preview only
#   bash reset-panel.sh --restore    # Restore from backup

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

PANEL_CONF="$HOME/.config/lxqt/panel.conf"
BACKUP_DIR="$HOME/.config/lxqt/backups"
STOCK_CONF="${SCRIPT_DIR}/../configs/dotfiles/lxqt/panel-stock.conf"

DRY_RUN=false
FORCE=false
RESTORE=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        --restore|-r) RESTORE=true ;;
        --help|-h)
            printf "Reset LXQt Panel to Stock Configuration\n\n"
            printf "Usage: bash reset-panel.sh [options]\n\n"
            printf "Options:\n"
            printf "  --force, -f      Skip confirmation prompt\n"
            printf "  --dry-run        Preview without making changes\n"
            printf "  --restore, -r    Restore from most recent backup\n"
            printf "  --help, -h       Show this help\n"
            exit 0
            ;;
    esac
done

print_header "LXQt Panel Reset"

# --- Verify stock config exists ---
if [ ! -f "$STOCK_CONF" ]; then
    log_error "Stock panel config not found: $STOCK_CONF"
    log_info "Ensure you are running from the project root or the scripts/ directory."
    exit 1
fi

# --- Restore mode ---
if [ "$RESTORE" = true ]; then
    mkdir -p "$BACKUP_DIR"
    LATEST_BACKUP=$(ls -t "$BACKUP_DIR"/panel.conf.* 2>/dev/null | head -1)
    if [ -z "$LATEST_BACKUP" ]; then
        log_error "No backup found in $BACKUP_DIR"
        exit 1
    fi

    log_info "Restoring from: $LATEST_BACKUP"

    if [ "$DRY_RUN" = true ]; then
        log_dim "[DRY-RUN] Would restore $LATEST_BACKUP -> $PANEL_CONF"
        exit 0
    fi

    mkdir -p "$(dirname "$PANEL_CONF")"
    cp "$LATEST_BACKUP" "$PANEL_CONF"
    log_ok "Panel restored from backup"
    log_info "Reconfigure labwc to apply: labwc -r"
    exit 0
fi

# --- Check current config exists ---
if [ ! -f "$PANEL_CONF" ]; then
    log_warn "No existing panel.conf found at $PANEL_CONF"
    log_info "Installing stock config fresh..."
    if [ "$DRY_RUN" = true ]; then
        log_dim "[DRY-RUN] Would install $STOCK_CONF -> $PANEL_CONF"
        exit 0
    fi
    mkdir -p "$(dirname "$PANEL_CONF")"
    cp "$STOCK_CONF" "$PANEL_CONF"
    log_ok "Stock panel config installed"
    log_info "Reconfigure labwc to apply: labwc -r"
    exit 0
fi

# --- Show current plugins ---
CURRENT_PLUGINS=$(grep "^plugins=" "$PANEL_CONF" | head -1 | cut -d= -f2)
STOCK_PLUGINS=$(grep "^plugins=" "$STOCK_CONF" | head -1 | cut -d= -f2)

printf "\n${BOLD}Current panel plugins:${RST}  %s\n" "$CURRENT_PLUGINS"
printf "${BOLD}Stock panel plugins:${RST}    %s\n\n" "$STOCK_PLUGINS"

# --- Confirmation ---
if [ "$FORCE" != true ] && [ "$DRY_RUN" != true ]; then
    printf "${BOLD}This will:${RST}\n"
    printf "  1. Backup current panel.conf to %s/panel.conf.<timestamp>\n" "$BACKUP_DIR"
    printf "  2. Replace panel.conf with the stock configuration\n"
    printf "  3. You will need to reconfigure labwc (labwc -r) to apply\n\n"
    printf "${BOLD}Continue? [y/N] ${RST}"
    read -r confirm
    case "$confirm" in
        y|Y|yes|YES) ;;
        *) log_info "Aborted."; exit 0 ;;
    esac
fi

# --- Backup current config ---
mkdir -p "$BACKUP_DIR"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
BACKUP_FILE="$BACKUP_DIR/panel.conf.$TIMESTAMP"

if [ "$DRY_RUN" = true ]; then
    log_dim "[DRY-RUN] Would backup $PANEL_CONF -> $BACKUP_FILE"
    log_dim "[DRY-RUN] Would replace $PANEL_CONF with $STOCK_CONF"
    exit 0
fi

cp "$PANEL_CONF" "$BACKUP_FILE"
log_ok "Backed up to $BACKUP_FILE"

# --- Replace with stock ---
cp "$STOCK_CONF" "$PANEL_CONF"
log_ok "Panel reset to stock configuration"

# --- Summary ---
printf "\n"
print_summary "Panel Reset" "ok" "Backup: $BACKUP_FILE"
printf "\n${BOLD}Next steps:${RST}\n"
printf "  1. Reconfigure:  ${GREEN}labwc -r${RST}\n"
printf "  2. Or restart:    ${GREEN}lxqt-panel -d &${RST}\n"
printf "  3. To restore:    ${GREEN}bash reset-panel.sh --restore${RST}\n"
