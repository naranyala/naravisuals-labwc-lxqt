#!/bin/bash
# Naravisuals — XWayland Fix
# ============================
# Fixes common XWayland issues for legacy X11 apps on Wayland.
# Handles toolkit backend, scaling, and display variable issues.
#
# Usage:
#   bash fix-xwayland.sh              # Auto-fix all issues
#   bash fix-xwayland.sh --status     # Check current state
#   bash fix-xwayland.sh --scale <val>  # Set XWayland scale factor

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

ACTION="fix"
SCALE_VAL=""

for arg in "$@"; do
  case "$arg" in
    --status) ACTION="status" ;;
    --scale)  shift; ACTION="scale"; SCALE_VAL="$1" ;;
    --help|-h)
      printf "XWayland Fix\n\n"
      printf "Usage: bash fix-xwayland.sh [options]\n\n"
      printf "Options:\n"
      printf "  --status          Check current XWayland state\n"
      printf "  --scale <val>     Set XWayland scale (e.g., 1.5, 2)\n"
      printf "  (no args)         Auto-fix common issues\n"
      exit 0
      ;;
  esac
done

print_header "XWayland Fix"

# ---- Status Check ----
check_status() {
  log_step "XWayland Status"

  # Check if XWayland is running
  if pgrep -x Xwayland &>/dev/null; then
    log_ok "XWayland is running"
  else
    log_warn "XWayland is not running"
  fi

  # Check DISPLAY
  if [ -n "${DISPLAY:-}" ]; then
    log_ok "DISPLAY=$DISPLAY"
  else
    log_warn "DISPLAY not set"
  fi

  # Check WAYLAND_DISPLAY
  if [ -n "${WAYLAND_DISPLAY:-}" ]; then
    log_ok "WAYLAND_DISPLAY=$WAYLAND_DISPLAY"
  else
    log_warn "WAYLAND_DISPLAY not set"
  fi

  # Check toolkit backend
  log_step "Toolkit Environment"
  for var in GDK_BACKEND QT_QPA_PLATFORM QT_WAYLAND_DISABLE_WINDOWDECORATION XDG_SESSION_TYPE; do
    val="${!var:-not set}"
    printf "  %-35s %s\n" "$var" "$val"
  done

  # Check XWayland display number
  if pgrep -x Xwayland &>/dev/null; then
    local xdisp
    xdisp=$(pgrep -a Xwayland | head -1 | grep -oP ':\d+' | head -1)
    [ -n "$xdisp" ] && log_info "XWayland display: $xdisp"
  fi
}

# ---- Fix ----
fix_xwayland() {
  log_step "Fixing XWayland issues"

  # 1. Ensure XWayland is enabled for compositor
  log_info "Checking compositor XWayland support..."

  local compositor=""
  if [ -f "$HOME/.config/lxqt/session.conf" ]; then
    compositor=$(grep "^compositor=" "$HOME/.config/lxqt/session.conf" 2>/dev/null | cut -d= -f2)
  fi

  case "$compositor" in
    hyprland)
      # Hyprland enables XWayland by default
      log_ok "Hyprland has XWayland enabled by default"
      ;;
    sway)
      # Check sway config
      local sway_config="$HOME/.config/sway/config"
      if [ -f "$sway_config" ] && ! grep -q "xwayland" "$sway_config"; then
        log_info "Adding xwayland enable to sway config"
        echo "xwayland enable" >> "$sway_config"
        log_ok "Added xwayland to sway config"
      fi
      ;;
    labwc)
      # Labwc should have XWayland if built with it
      log_dim "Labwc XWayland support depends on build options"
      ;;
  esac

  # 2. Fix GTK backend for X11 apps
  log_step "Fixing GTK toolkit backend"

  ENV_FILE="$HOME/.config/labwc/environment"
  if [ -f "$ENV_FILE" ]; then
    # Remove old GDK_BACKEND if set to wayland-only
    if grep -q "^GDK_BACKEND=" "$ENV_FILE"; then
      sed -i 's/^GDK_BACKEND=.*/# GDK_BACKEND let Wayland decide/' "$ENV_FILE"
      log_ok "Removed restrictive GDK_BACKEND setting"
    fi
  fi

  # 3. Fix Qt platform plugin
  log_step "Fixing Qt platform plugin"

  ENV_FILE="$HOME/.config/labwc/environment"
  if [ -f "$ENV_FILE" ]; then
    if ! grep -q "QT_QPA_PLATFORM" "$ENV_FILE"; then
      echo "# Qt will auto-detect Wayland or X11" >> "$ENV_FILE"
      log_ok "Qt platform auto-detection enabled"
    fi
  fi

  # 4. Set DISPLAY if missing (for XWayland)
  log_step "Ensuring DISPLAY variable"

  ENV_FILE="$HOME/.config/labwc/environment"
  if [ -f "$ENV_FILE" ]; then
    if ! grep -q "^DISPLAY=" "$ENV_FILE"; then
      echo "DISPLAY=:0" >> "$ENV_FILE"
      log_ok "Added DISPLAY=:0 to environment"
    fi
  fi

  # 5. Fix XDG_SESSION_TYPE
  log_step "Setting session type"

  ENV_FILE="$HOME/.config/labwc/environment"
  if [ -f "$ENV_FILE" ]; then
    if ! grep -q "XDG_SESSION_TYPE" "$ENV_FILE"; then
      echo "XDG_SESSION_TYPE=wayland" >> "$ENV_FILE"
      log_ok "Added XDG_SESSION_TYPE=wayland"
    fi
  fi

  log_ok "XWayland fixes applied. Restart compositor to take effect."
}

# ---- Set Scale ----
set_scale() {
  local scale="$1"

  log_step "Setting XWayland scale to $scale"

  # Environment variable approach
  ENV_FILE="$HOME/.config/labwc/environment"
  if [ -f "$ENV_FILE" ]; then
    if grep -q "GDK_SCALE" "$ENV_FILE"; then
      sed -i "s/^GDK_SCALE=.*/GDK_SCALE=$scale/" "$ENV_FILE"
    else
      echo "GDK_SCALE=$scale" >> "$ENV_FILE"
    fi
    log_ok "Set GDK_SCALE=$scale"
  fi

  # For Qt apps
  if [ -f "$ENV_FILE" ]; then
    if grep -q "QT_SCALE_FACTOR" "$ENV_FILE"; then
      sed -i "s/^QT_SCALE_FACTOR=.*/QT_SCALE_FACTOR=$scale/" "$ENV_FILE"
    else
      echo "QT_SCALE_FACTOR=$scale" >> "$ENV_FILE"
    fi
    log_ok "Set QT_SCALE_FACTOR=$scale"
  fi

  log_info "Restart compositor to apply scaling"
}

# ---- Main ----
case "$ACTION" in
  status) check_status ;;
  fix)    fix_xwayland ;;
  scale)  set_scale "$SCALE_VAL" ;;
esac
