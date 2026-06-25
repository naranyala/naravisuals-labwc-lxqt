#!/bin/bash
# Naravisuals — Install LXQt Panel Plugins
# ==========================================
# Installs naravisuals panel plugins to system directories.
#
# Usage:
#   bash install-plugins.sh              # Install all
#   bash install-plugins.sh --list       # List installed plugins
#   bash install-plugins.sh --prefix DIR # Install to custom prefix
#   bash install-plugins.sh <plugin>     # Install specific plugin

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLUGIN_DIR="$SCRIPT_DIR/plugins"
BUILD_DIR="$PLUGIN_DIR/build"
PREFIX="${PREFIX:-/usr}"
SO_DIR="$PREFIX/lib/x86_64-linux-gnu/lxqt-panel"
DESK_DIR="$PREFIX/share/lxqt/lxqt-panel"
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
    --prefix|-p) shift; PREFIX="$1"; SO_DIR="$PREFIX/lib/x86_64-linux-gnu/lxqt-panel"; DESK_DIR="$PREFIX/share/lxqt/lxqt-panel" ;;
    --help|-h)
      printf "Install LXQt Panel Plugins\n\n"
      printf "Usage: bash install-plugins.sh [options] [plugin]\n\n"
      printf "Options:\n"
      printf "  --list, -l         List installed plugins\n"
      printf "  --prefix, -p DIR   Install to custom prefix (default: /usr)\n"
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

# ---- List Installed ----
if [ "$LIST" = true ]; then
  log_info "Installed naravisuals plugins:"
  for p in "${PLUGINS[@]}"; do
    so="$SO_DIR/lib${p}.so"
    desk="$DESK_DIR/${p}.desktop"
    if [ -f "$so" ]; then
      size=$(du -h "$so" 2>/dev/null | cut -f1)
      log_ok "  $p  ($size)"
    else
      log_warn "  $p  (not installed)"
    fi
  done
  exit 0
fi

# ---- Check Prereqs ----
if ! [ -d "$BUILD_DIR" ]; then
  log_error "Build directory not found: $BUILD_DIR"
  log_info "Run: bash build-plugins.sh --install"
  exit 1
fi

if ! [ -f "$SO_DIR" ]; then
  log_info "Creating plugin directory: $SO_DIR"
  sudo mkdir -p "$SO_DIR"
fi

if ! [ -f "$DESK_DIR" ]; then
  log_info "Creating desktop directory: $DESK_DIR"
  sudo mkdir -p "$DESK_DIR"
fi

# ---- Install ----
echo ""
printf "${BOLD}Installing LXQt Panel Plugins${RST}\n"
printf "%s\n" "────────────────────────────────────────"

install_plugin() {
  local name="$1"
  local so_src="$BUILD_DIR/plugins/${name}/lib${name}.so"
  local desk_src="$PLUGIN_DIR/${name}/${name}.desktop"

  # Try alternate SO location
  [ ! -f "$so_src" ] && so_src="$BUILD_DIR/lib${name}.so"

  # Install .so
  if [ -f "$so_src" ]; then
    sudo cp "$so_src" "$SO_DIR/"
    log_ok "Installed: lib${name}.so -> $SO_DIR/"
  else
    log_warn "SO not found for $name, skipping"
    return 1
  fi

  # Install .desktop
  if [ -f "$desk_src" ]; then
    sudo cp "$desk_src" "$DESK_DIR/"
    log_ok "Installed: ${name}.desktop -> $DESK_DIR/"
  else
    log_warn "Desktop file not found for $name"
  fi
}

if [ -n "$TARGET" ]; then
  install_plugin "$TARGET"
else
  ok=0
  fail=0
  for p in "${PLUGINS[@]}"; do
    if install_plugin "$p"; then
      ok=$((ok + 1))
    else
      fail=$((fail + 1))
    fi
  done
  echo ""
  log_info "Installed: $ok  Failed: $fail"
fi

echo ""
printf "${BOLD}Next steps:${RST}\n"
printf "  1. Restart lxqt-panel:  killall lxqt-panel && lxqt-panel &\n"
printf "  2. Add plugins:         Right-click panel → Add/Remove Plugins\n"
