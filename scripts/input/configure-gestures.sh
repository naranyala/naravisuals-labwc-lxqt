#!/bin/bash
# Naravisuals — Configure Gestures
# ===================================
# Configures libinput gestures for Wayland (swipe, pinch, tap).
#
# Usage:
#   bash configure-gestures.sh              # Interactive setup
#   bash configure-gestures.sh --list       # List supported gestures
#   bash configure-gestures.sh --defaults   # Apply default config
#   bash configure-gestures.sh --file <path>  # Use custom gesture file

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

ACTION="setup"
GESTURE_FILE=""

for arg in "$@"; do
  case "$arg" in
    --list) ACTION="list" ;;
    --defaults) ACTION="defaults" ;;
    --file) shift; ACTION="file"; GESTURE_FILE="$1" ;;
    --help|-h)
      printf "Gesture Configuration\n\n"
      printf "Usage: bash configure-gestures.sh [options]\n\n"
      printf "Options:\n"
      printf "  --list         List supported gestures\n"
      printf "  --defaults     Apply default gesture config\n"
      printf "  --file <path>  Use custom gesture file\n"
      printf "  (no args)      Interactive setup\n"
      exit 0
      ;;
  esac
done

print_header "Gesture Configuration"

# ---- Check Dependencies ----
check_deps() {
  local missing=()

  if ! cmd_exists "libinput"; then
    missing+=("libinput")
  fi

  # Check for gesture daemon
  if ! cmd_exists "fusuma" && ! cmd_exists "swaygestures" && ! cmd_exists "libinput-gestures"; then
    missing+=("fusuma or libinput-gestures")
  fi

  if [ ${#missing[@]} -gt 0 ]; then
    log_error "Missing: ${missing[*]}"
    log_info "Install libinput-gestures: pip3 install libinput-gestures"
    log_info "Install fusuma: gem install fusuma"
    return 1
  fi
}

# ---- List Gestures ----
list_gestures() {
  log_step "Available gestures"

  printf "\n${BOLD}Swipe Gestures:${RST}\n"
  printf "  3-finger swipe up/down    → Workspace switch\n"
  printf "  3-finger swipe left/right → Browser back/forward\n"
  printf "  4-finger swipe up/down    → App launcher\n"
  printf "  4-finger swipe left/right → Window snap\n"

  printf "\n${BOLD}Pinch Gestures:${RST}\n"
  printf "  2-finger pinch in   → Zoom out / minimize\n"
  printf "  2-finger pinch out  → Zoom in / maximize\n"
  printf "  2-finger rotate     → Rotate window (if supported)\n"

  printf "\n${BOLD}Tap Gestures:${RST}\n"
  printf "  3-finger tap        → Middle click (paste)\n"
  printf "  4-finger tap        → App launcher\n"

  # Show detected devices
  log_step "Touchpad devices"
  if cmd_exists "libinput"; then
    libinput list-devices 2>/dev/null | grep -A2 -i "touchpad" | head -12
  fi
}

# ---- Create Config ----
create_config() {
  local config_dir=""
  local config_file=""

  # Detect which gesture tool is installed
  if cmd_exists "libinput-gestures"; then
    config_dir="$HOME/.config"
    config_file="$config_dir/libinput-gestures.conf"
  elif cmd_exists "fusuma"; then
    config_dir="$HOME/.config"
    config_file="$config_dir/fusuma/config.yml"
  elif cmd_exists "swaygestures"; then
    config_dir="$HOME/.config/sway"
    config_file="$config_dir/gestures.conf"
  fi

  if [ -z "$config_file" ]; then
    log_error "No gesture daemon found"
    return 1
  fi

  mkdir -p "$config_dir"

  if [ -f "$config_file" ]; then
    log_warn "Config already exists: $config_file"
    log_info "Backing up to ${config_file}.bak"
    cp "$config_file" "${config_file}.bak"
  fi

  log_step "Creating gesture config"

  # libinput-gestures format
  if cmd_exists "libinput-gestures"; then
    cat > "$config_file" << 'GESTURES'
# Naravisuals Gesture Configuration
# libinput-gestures.conf

# ---- Workspace switching (3-finger swipe) ----
gesture swipe up 3 swaymsg workspace next_on_output
gesture swipe down 3 swaymsg workspace prev_on_output

# ---- Browser navigation (3-finger horizontal) ----
gesture swipe left 3 xdotool key alt+Left
gesture swipe right 3 xdotool key alt+Right

# ---- App launcher (4-finger swipe up) ----
gesture swipe up 4 wofi --show drun

# ---- Window snap (4-finger horizontal) ----
gesture swipe left 4 swaymsg move container to workspace prev
gesture swipe right 4 swaymsg move container to workspace next

# ---- Zoom (2-finger pinch) ----
gesture pinch in 2 xdotool key ctrl+minus
gesture pinch out 2 xdotool key ctrl+plus

# ---- Three-finger tap = middle click ----
gesture tap 3 xdotool key click 2

# ---- Four-finger tap = launcher ----
gesture tap 4 wofi --show drun
GESTURES
    log_ok "Created libinput-gestures config"
  fi

  # fusuma format
  if cmd_exists "fusuma"; then
    cat > "$config_file" << 'FUSUMA'
# Naravisuals Gesture Configuration
# fusuma config.yml

threshold:
  swipe: 0.3
  pinch: 0.1

swipe:
  3:
    up:
      command: swaymsg workspace next_on_output
    down:
      command: swaymsg workspace prev_on_output
    left:
      command: xdotool key alt+Left
    right:
      command: xdotool key alt+Right
  4:
    up:
      command: wofi --show drun
    left:
      command: swaymsg move container to workspace prev
    right:
      command: swaymsg move container to workspace next

pinch:
  2:
    in:
      command: xdotool key ctrl+minus
    out:
      command: xdotool key ctrl+plus
FUSUMA
    log_ok "Created fusuma config"
  fi

  log_info "Config: $config_file"
}

# ---- Apply Defaults ----
apply_defaults() {
  check_deps || exit 1
  create_config

  # Enable gesture daemon
  AUTOSTART="$HOME/.config/labwc/autostart"
  if [ -f "$AUTOSTART" ]; then
    if ! grep -q "libinput-gestures\|fusuma" "$AUTOSTART"; then
      log_step "Adding gesture daemon to autostart"
      if cmd_exists "libinput-gestures"; then
        echo "libinput-gestures-setup start &" >> "$AUTOSTART"
      elif cmd_exists "fusuma"; then
        echo "fusuma &" >> "$AUTOSTART"
      fi
      log_ok "Added gesture daemon to autostart"
    fi
  fi

  print_summary "Gesture Setup" "ok" "Gestures configured. Restart session to apply."
}

# ---- Main ----
case "$ACTION" in
  list)     list_gestures ;;
  defaults) apply_defaults ;;
  file)
    if [ -f "$GESTURE_FILE" ]; then
      config_target=""
      if cmd_exists "libinput-gestures"; then
        config_target="$HOME/.config/libinput-gestures.conf"
      elif cmd_exists "fusuma"; then
        config_target="$HOME/.config/fusuma/config.yml"
      fi
      if [ -n "$config_target" ]; then
        mkdir -p "$(dirname "$config_target")"
        cp "$GESTURE_FILE" "$config_target"
        log_ok "Installed gesture config: $config_target"
      fi
    else
      log_error "File not found: $GESTURE_FILE"
      exit 1
    fi
    ;;
  setup)
    check_deps || exit 1
    apply_defaults
    ;;
esac
