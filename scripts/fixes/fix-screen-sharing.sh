#!/bin/bash
# Naravisuals — Fix Screen Sharing
# ====================================
# Fixes XDG Desktop Portal screen sharing for Wayland.
# Common issue: apps like Discord/Zoom/OBS can't capture screen.
#
# Usage:
#   bash fix-screen-sharing.sh              # Auto-fix
#   bash fix-screen-sharing.sh --status     # Check portal status
#   bash fix-screen-sharing.sh --pipewire   # Fix PipeWire setup

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

ACTION="fix"

for arg in "$@"; do
  case "$arg" in
    --status) ACTION="status" ;;
    --pipewire) ACTION="pipewire" ;;
    --help|-h)
      printf "Screen Sharing Fix\n\n"
      printf "Usage: bash fix-screen-sharing.sh [options]\n\n"
      printf "Options:\n"
      printf "  --status      Check portal status\n"
      printf "  --pipewire    Fix PipeWire setup only\n"
      printf "  (no args)     Auto-fix all issues\n"
      exit 0
      ;;
  esac
done

print_header "Screen Sharing Fix"

PM="$(detect_pm)"

# ---- Status Check ----
check_status() {
  log_step "Portal Status"

  # Check xdg-desktop-portal
  if pgrep -x xdg-desktop-portal &>/dev/null; then
    log_ok "xdg-desktop-portal is running"
  else
    log_warn "xdg-desktop-portal is NOT running"
  fi

  # Check xdg-desktop-portal-wlr
  if pgrep -x xdg-desktop-portal-wlr &>/dev/null; then
    log_ok "xdg-desktop-portal-wlr is running"
  else
    log_warn "xdg-desktop-portal-wlr is NOT running (needed for wlroots compositors)"
  fi

  # Check PipeWire
  if pgrep -x pipewire &>/dev/null; then
    log_ok "PipeWire is running"
  else
    log_warn "PipeWire is NOT running"
  fi

  # Check portal environment
  log_step "Environment"
  for var in XDG_CURRENT_DESKTOP XDG_SESSION_DESKTOP XDG_SESSION_TYPE; do
    val="${!var:-not set}"
    printf "  %-30s %s\n" "$var" "$val"
  done

  # Check portal config
  log_step "Portal Configuration"
  local portal_conf="$HOME/.config/xdg-desktop-portal/portals.conf"
  if [ -f "$portal_conf" ]; then
    log_ok "Portal config: $portal_conf"
    cat "$portal_conf"
  else
    log_dim "No portal config found (using defaults)"
  fi
}

# ---- Fix PipeWire ----
fix_pipewire() {
  log_step "Fixing PipeWire"

  case "$PM" in
    apt)
      sudo apt-get install -y pipewire pipewire-pulse wireplumber 2>/dev/null || \
      sudo apt-get install -y pipewire pipewire-pulse pipewire-media-session 2>/dev/null
      ;;
    dnf)
      sudo dnf install -y pipewire pipewire-pulse wireplumber 2>/dev/null
      ;;
    pacman)
      sudo pacman -S --noconfirm pipewire pipewire-pulse wireplumber 2>/dev/null
      ;;
    zypper)
      sudo zypper install -y pipewire pipewire-pulse wireplumber 2>/dev/null
      ;;
  esac

  # Ensure user services are enabled
  if cmd_exists "systemctl"; then
    systemctl --user enable --now pipewire 2>/dev/null || true
    systemctl --user enable --now pipewire-pulse 2>/dev/null || true
    systemctl --user enable --now wireplumber 2>/dev/null || true
  fi

  if pgrep -x pipewire &>/dev/null; then
    log_ok "PipeWire is running"
  else
    log_warn "PipeWire may need a reboot to start"
  fi
}

# ---- Fix Portals ----
fix_portals() {
  log_step "Fixing XDG Desktop Portal"

  # Install portal packages
  case "$PM" in
    apt)
      sudo apt-get install -y xdg-desktop-portal xdg-desktop-portal-wlr 2>/dev/null
      sudo apt-get install -y xdg-desktop-portal-gtk 2>/dev/null
      ;;
    dnf)
      sudo dnf install -y xdg-desktop-portal xdg-desktop-portal-wlr 2>/dev/null
      sudo dnf install -y xdg-desktop-portal-gtk 2>/dev/null
      ;;
    pacman)
      sudo pacman -S --noconfirm xdg-desktop-portal xdg-desktop-portal-wlr 2>/dev/null
      sudo pacman -S --noconfirm xdg-desktop-portal-gtk 2>/dev/null
      ;;
    zypper)
      sudo zypper install -y xdg-desktop-portal xdg-desktop-portal-wlr 2>/dev/null
      ;;
  esac

  # Create portal config for labwc
  PORTAL_DIR="$HOME/.config/xdg-desktop-portal"
  mkdir -p "$PORTAL_DIR"

  PORTAL_CONF="$PORTAL_DIR/portals.conf"
  if [ ! -f "$PORTAL_CONF" ]; then
    cat > "$PORTAL_CONF" << 'PORTAL'
[preferred]
default=wlr;gtk
org.freedesktop.impl.portal.Screenshot=wlr
org.freedesktop.impl.portal.ScreenCast=wlr
org.freedesktop.impl.portal.FileChooser=gtk
PORTAL
    log_ok "Created portal config"
  else
    log_dim "Portal config already exists"
  fi

  # Kill and restart portals
  log_step "Restarting portal services"
  pkill -x xdg-desktop-portal 2>/dev/null || true
  pkill -x xdg-desktop-portal-wlr 2>/dev/null || true
  sleep 1

  xdg-desktop-portal &>/dev/null &
  sleep 1
  xdg-desktop-portal-wlr &>/dev/null &

  log_ok "Portal services restarted"
}

# ---- Fix Environment ----
fix_environment() {
  log_step "Fixing environment variables"

  ENV_FILE="$HOME/.config/labwc/environment"

  # Ensure XDG_CURRENT_DESKTOP
  if [ -f "$ENV_FILE" ]; then
    if ! grep -q "XDG_CURRENT_DESKTOP" "$ENV_FILE"; then
      echo "XDG_CURRENT_DESKTOP=wlroots" >> "$ENV_FILE"
      log_ok "Added XDG_CURRENT_DESKTOP=wlroots"
    fi

    # Ensure XDG_SESSION_TYPE
    if ! grep -q "XDG_SESSION_TYPE" "$ENV_FILE"; then
      echo "XDG_SESSION_TYPE=wayland" >> "$ENV_FILE"
      log_ok "Added XDG_SESSION_TYPE=wayland"
    fi
  fi
}

# ---- Main ----
case "$ACTION" in
  status)   check_status ;;
  pipewire) fix_pipewire ;;
  fix)
    fix_pipewire
    fix_portals
    fix_environment
    log_ok "Screen sharing should now work. Restart your session."
    printf "\n${BOLD}Test with:${RST}\n"
    printf "  xdg-desktop-portal-wlr\n"
    printf "  (then try screen share in Discord/Zoom/OBS)\n"
    ;;
esac
