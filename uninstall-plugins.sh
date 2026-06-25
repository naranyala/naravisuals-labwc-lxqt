#!/bin/bash
# Naravisuals — Uninstall LXQt Panel Plugins
# ============================================
# Removes naravisuals panel plugins from system directories.
#
# Usage:
#   bash uninstall-plugins.sh              # Uninstall all
#   bash uninstall-plugins.sh --list       # List what would be removed
#   bash uninstall-plugins.sh --prefix DIR # Uninstall from custom prefix
#   bash uninstall-plugins.sh <plugin>     # Uninstall specific plugin
#   bash uninstall-plugins.sh --purge      # Also remove settings/data

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PREFIX="${PREFIX:-/usr}"
SO_DIR="$PREFIX/lib/x86_64-linux-gnu/lxqt-panel"
DESK_DIR="$PREFIX/share/lxqt/lxqt-panel"
DATA_DIR="$HOME/.local/share/naravisuals"
PURGE=false
TARGET=""

PLUGINS=(
  naravisuals-weather-widget
  naravisuals-cpu-monitor
  naravisuals-sticky-notes
  naravisuals-color-picker
  naravisuals-screenshot
  naravisuals-nightlight
  naravisuals-updates
)

# ---- Colors ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
RST='\033[0m'

log_info()  { printf "${BLUE}::${RST} %s\n" "$*"; }
log_ok()    { printf "${GREEN}✓${RST} %s\n" "$*"; }
log_warn()  { printf "${YELLOW}⚠${RST} %s\n" "$*"; }
log_error() { printf "${RED}✗${RST} %s\n" "$*" >&2; }

# ---- Parse Args ----
LIST=false
for arg in "$@"; do
  case "$arg" in
    --list|-l) LIST=true ;;
    --purge) PURGE=true ;;
    --prefix|-p) shift; PREFIX="$1"; SO_DIR="$PREFIX/lib/x86_64-linux-gnu/lxqt-panel"; DESK_DIR="$PREFIX/share/lxqt/lxqt-panel" ;;
    --help|-h)
      printf "Uninstall LXQt Panel Plugins\n\n"
      printf "Usage: bash uninstall-plugins.sh [options] [plugin]\n\n"
      printf "Options:\n"
      printf "  --list, -l         List what would be removed\n"
      printf "  --purge            Also remove user settings and data\n"
      printf "  --prefix, -p DIR   Uninstall from custom prefix\n"
      printf "  --help, -h         Show this help\n\n"
      printf "Plugins:\n"
      for p in "${PLUGINS[@]}"; do
        printf "  %s\n" "$p"
      done
      exit 0
      ;;
    naravisuals-*) TARGET="$arg" ;;
  esac
done

# ---- List / Dry Run ----
if [ "$LIST" = true ]; then
  log_info "Files that would be removed:"
  for p in "${PLUGINS[@]}"; do
    so="$SO_DIR/lib${p}.so"
    desk="$DESK_DIR/${p}.desktop"
    [ -f "$so" ] && log_ok "  $so"
    [ -f "$desk" ] && log_ok "  $desk"
  done
  if [ "$PURGE" = true ]; then
    log_info "Data files that would be removed:"
    [ -d "$DATA_DIR" ] && log_ok "  $DATA_DIR/"
  fi
  exit 0
fi

# ---- Uninstall ----
echo ""
printf "${BOLD}Uninstalling LXQt Panel Plugins${RST}\n"
printf "%s\n" "────────────────────────────────────────"

uninstall_plugin() {
  local name="$1"
  local so="$SO_DIR/lib${name}.so"
  local desk="$DESK_DIR/${name}.desktop"

  if [ -f "$so" ]; then
    sudo rm -f "$so"
    log_ok "Removed: $so"
  fi

  if [ -f "$desk" ]; then
    sudo rm -f "$desk"
    log_ok "Removed: $desk"
  fi
}

if [ -n "$TARGET" ]; then
  uninstall_plugin "$TARGET"
else
  count=0
  for p in "${PLUGINS[@]}"; do
    uninstall_plugin "$p"
    count=$((count + 1))
  done
  log_info "Uninstalled $count plugins"
fi

# ---- Purge user data ----
if [ "$PURGE" = true ] && [ -d "$DATA_DIR" ]; then
  log_warn "Removing user data: $DATA_DIR"
  rm -rf "$DATA_DIR"
  log_ok "Removed: $DATA_DIR"
fi

# ---- Remove panel config entries ----
PANEL_CONF="$HOME/.config/lxqt/panel.conf"
if [ -f "$PANEL_CONF" ]; then
  backup="$PANEL_CONF.bak.$(date +%Y%m%d-%H%M%S)"
  cp "$PANEL_CONF" "$backup"
  log_info "Panel config backed up to: $backup"

  # Remove naravisuals plugin entries from panel.conf
  for p in "${PLUGINS[@]}"; do
    sed -i "/lib${p}\.so/d" "$PANEL_CONF" 2>/dev/null || true
  done
  log_ok "Cleaned panel.conf entries"
fi

echo ""
printf "${BOLD}Done.${RST}\n"
printf "  Restart lxqt-panel:  killall lxqt-panel && lxqt-panel &\n"
