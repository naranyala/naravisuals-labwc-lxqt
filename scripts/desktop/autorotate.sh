#!/bin/bash
# Naravisuals — Auto-Rotate
# ==========================
# Auto-rotates screen based on accelerometer orientation.
# Uses iio-sensor-proxy for tablet/laptop screens.
#
# Usage:
#   bash autorotate.sh              # Start auto-rotation daemon
#   bash autorotate.sh off          # Stop auto-rotation
#   bash autorotate.sh status       # Check if running
#   bash autorotate.sh once <dir>   # Rotate once: left|right|normal|inverted

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

PIDFILE="${XDG_RUNTIME_DIR:-/tmp}/naravisuals-autorotate.pid"

# ---- Check Dependencies ----
check_deps() {
  local missing=()

  if ! cmd_exists "iio-sensor-proxy"; then
    missing+=("iio-sensor-proxy")
  fi

  # Need at least one output tool
  if ! cmd_exists "wlr-randr" && ! cmd_exists "kscreen-doctor"; then
    missing+=("wlr-randr or kscreen-doctor")
  fi

  if [ ${#missing[@]} -gt 0 ]; then
    log_error "Missing: ${missing[*]}"
    log_info "Install: sudo apt install iio-sensor-proxy wlr-randr"
    return 1
  fi
}

# ---- Detect Current Orientation ----
detect_orientation() {
  if cmd_exists "iio-sensor-proxy"; then
    # Use busctl to query sensor proxy
    local orient
    orient=$(busctl call org.freedesktop.SensorsService /org/freedesktop/SensorsService org.freedesktop.SensorsService GetOrientation 2>/dev/null | grep -oP '"[^"]*"' | tr -d '"')

    case "$orient" in
      normal)    echo "normal" ;;
      left)      echo "left" ;;
      right)     echo "right" ;;
      upside-down) echo "inverted" ;;
      *)         echo "unknown" ;;
    esac
  else
    echo "unknown"
  fi
}

# ---- Apply Rotation ----
apply_rotation() {
  local orientation="$1"

  case "$orientation" in
    normal)
      if cmd_exists "wlr-randr"; then
        wlr-randr --transform normal 2>/dev/null
      elif cmd_exists "kscreen-doctor"; then
        kscreen-doctor output.`wlr-randr 2>/dev/null | head -1 | awk '{print $1}'`.transform.normal 2>/dev/null
      fi
      ;;
    left)
      if cmd_exists "wlr-randr"; then
        wlr-randr --transform 90 2>/dev/null
      elif cmd_exists "kscreen-doctor"; then
        kscreen-doctor output.`wlr-randr 2>/dev/null | head -1 | awk '{print $1}'`.transform.90 2>/dev/null
      fi
      ;;
    right)
      if cmd_exists "wlr-randr"; then
        wlr-randr --transform 270 2>/dev/null
      elif cmd_exists "kscreen-doctor"; then
        kscreen-doctor output.`wlr-randr 2>/dev/null | head -1 | awk '{print $1}'`.transform.270 2>/dev/null
      fi
      ;;
    inverted|upside-down)
      if cmd_exists "wlr-randr"; then
        wlr-randr --transform 180 2>/dev/null
      elif cmd_exists "kscreen-doctor"; then
        kscreen-doctor output.`wlr-randr 2>/dev/null | head -1 | awk '{print $1}'`.transform.180 2>/dev/null
      fi
      ;;
    *)
      log_warn "Unknown orientation: $orientation"
      return 1
      ;;
  esac
}

# ---- Daemon Loop ----
run_daemon() {
  log_step "Starting auto-rotate daemon"

  # Start iio-sensor-proxy if not running
  if ! pgrep -x iio-sensor-proxy &>/dev/null; then
    log_info "Starting iio-sensor-proxy..."
    sudo systemctl start iio-sensor-proxy 2>/dev/null || {
      sudo iio-sensor-proxy &>/dev/null &
    }
    sleep 1
  fi

  log_ok "Auto-rotate active. Ctrl+C to stop."

  local last_orientation=""
  while true; do
    local orient
    orient="$(detect_orientation)"

    if [ "$orient" != "unknown" ] && [ "$orient" != "$last_orientation" ]; then
      log_info "Orientation changed: $orient"
      apply_rotation "$orient"
      last_orientation="$orient"
    fi

    sleep 1
  done
}

# ---- Start ----
start_daemon() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    log_info "Auto-rotate already running (PID: $(cat "$PIDFILE"))"
    exit 0
  fi

  check_deps || exit 1

  run_daemon &
  echo $! > "$PIDFILE"
  log_ok "Auto-rotate daemon started (PID: $!)"
}

# ---- Stop ----
stop_daemon() {
  if [ -f "$PIDFILE" ]; then
    local pid
    pid=$(cat "$PIDFILE")
    kill "$pid" 2>/dev/null && log_ok "Auto-rotate stopped" || log_info "Not running"
    rm -f "$PIDFILE"
  else
    pkill -f "naravisuals-autorotate" 2>/dev/null && log_ok "Auto-rotate stopped" || log_info "Not running"
  fi

  # Also stop iio-sensor-proxy
  if cmd_exists "iio-sensor-proxy"; then
    sudo systemctl stop iio-sensor-proxy 2>/dev/null || true
  fi
}

# ---- Main ----
case "${1:-start}" in
  on|start|"")
    start_daemon
    ;;
  off|stop)
    stop_daemon
    ;;
  status)
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
      log_ok "Auto-rotate is running (PID: $(cat "$PIDFILE"))"
      local orient
      orient="$(detect_orientation)"
      log_info "Current orientation: $orient"
    else
      log_info "Auto-rotate is not running"
    fi
    ;;
  once)
    if [ -z "${2:-}" ]; then
      log_error "Specify orientation: left, right, normal, inverted"
      exit 1
    fi
    check_deps || exit 1
    apply_rotation "$2" && log_ok "Rotated to $2" || log_error "Rotation failed"
    ;;
  *)
    log_error "Unknown command: $1"
    printf "Usage: bash autorotate.sh [on|off|status|once <orientation>]\n"
    exit 1
    ;;
esac
