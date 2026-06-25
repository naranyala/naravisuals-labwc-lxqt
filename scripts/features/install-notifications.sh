#!/bin/bash
# Naravisuals — Install Notifications
# ======================================
# Sets up a notification daemon (dunst or mako) for Wayland.
#
# Usage:
#   bash install-notifications.sh              # Install dunst (default)
#   bash install-notifications.sh --mako       # Install mako instead
#   bash install-notifications.sh --list       # List options

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

DAEMON="dunst"

for arg in "$@"; do
  case "$arg" in
    --mako) DAEMON="mako" ;;
    --dunst) DAEMON="dunst" ;;
    --list)
      printf "Notification Daemons:\n"
      printf "  dunst  — Feature-rich, highly customizable (default)\n"
      printf "  mako   — Lightweight, Wayland-native, wlroots-friendly\n"
      exit 0
      ;;
    --help|-h)
      printf "Notification Daemon Setup\n\n"
      printf "Usage: bash install-notifications.sh [options]\n\n"
      printf "Options:\n"
      printf "  --dunst    Install dunst (default)\n"
      printf "  --mako     Install mako instead\n"
      printf "  --list     List available daemons\n"
      exit 0
      ;;
  esac
done

print_header "Notification Daemon Setup"

PM="$(detect_pm)"

# --- Install dunst ---
install_dunst() {
  log_step "Installing dunst"

  case "$PM" in
    apt)
      sudo apt-get install -y dunst 2>/dev/null
      ;;
    dnf)
      sudo dnf install -y dunst 2>/dev/null
      ;;
    pacman)
      sudo pacman -S --noconfirm dunst 2>/dev/null
      ;;
    zypper)
      sudo zypper install -y dunst 2>/dev/null
      ;;
    *)
      log_warn "Unknown package manager. Install dunst manually."
      return 1
      ;;
  esac

  if ! cmd_exists dunst; then
    log_error "dunst not found after install"
    return 1
  fi

  # Create config
  DUNST_DIR="$HOME/.config/dunst"
  mkdir -p "$DUNST_DIR"

  if [ ! -f "$DUNST_DIR/dunstrc" ]; then
    cat > "$DUNST_DIR/dunstrc" << 'DUNST'
[global]
    monitor = 0
    follow = mouse
    width = 350
    height = 100
    origin = top-right
    offset = 10x10
    scale = 0
    notification_limit = 5
    progress_bar = true
    progress_bar_height = 10
    progress_bar_frame_width = 1
    progress_bar_min_width = 150
    progress_bar_max_width = 300

    indicate_hidden = yes
    transparency = 0
    separator_height = 2
    padding = 12
    horizontal_padding = 12
    text_icon_padding = 0
    frame_width = 2
    frame_color = "#333333"
    gap_size = 4
    separator_color = frame
    sort = yes
    idle_threshold = 120

    font = Monospace 10
    line_height = 0
    markup = full
    format = "<b>%s</b>\n%b"
    alignment = left
    vertical_alignment = center
    show_age_threshold = 60
    ellipsize = middle
    ignore_newline = no
    stack_duplicates = true
    hide_duplicate_count = false
    show_indicators = yes

    icon_position = left
    min_icon_size = 32
    max_icon_size = 64

[urgency_low]
    background = "#222222"
    foreground = "#888888"
    frame_color = "#333333"
    timeout = 5

[urgency_normal]
    background = "#222222"
    foreground = "#FFFFFF"
    frame_color = "#333333"
    timeout = 10

[urgency_critical]
    background = "#900000"
    foreground = "#FFFFFF"
    frame_color = "#FF0000"
    timeout = 0
DUNST
    log_ok "Created dunst config"
  else
    log_dim "dunst config already exists"
  fi
}

# --- Install mako ---
install_mako() {
  log_step "Installing mako"

  case "$PM" in
    apt)
      sudo apt-get install -y mako 2>/dev/null
      ;;
    dnf)
      sudo dnf install -y mako 2>/dev/null
      ;;
    pacman)
      sudo pacman -S --noconfirm mako 2>/dev/null
      ;;
    zypper)
      sudo zypper install -y mako 2>/dev/null
      ;;
    *)
      log_warn "Unknown package manager. Install mako manually."
      return 1
      ;;
  esac

  if ! cmd_exists mako; then
    log_error "mako not found after install"
    return 1
  fi

  # Create config
  MAKO_DIR="$HOME/.config/mako"
  mkdir -p "$MAKO_DIR"

  if [ ! -f "$MAKO_DIR/config" ]; then
    cat > "$MAKO_DIR/config" << 'MAKO'
width=350
height=100
margin=10
padding=12
border-size=2
border-color=#333333
border-radius=5
background-color=#222222
text-color=#FFFFFF
font=Monospace 10
max-visible=5
default-timeout=5000
layer=overlay
anchor=top-right
actions=1
icons=1
max-icon-size=64
MAKO
    log_ok "Created mako config"
  else
    log_dim "mako config already exists"
  fi
}

# --- Install selected daemon ---
case "$DAEMON" in
  dunst) install_dunst ;;
  mako)  install_mako ;;
esac

# --- Add to autostart ---
log_step "Adding to autostart"

AUTOSTART="$HOME/.config/labwc/autostart"
if [ -f "$AUTOSTART" ]; then
  # Remove old notification daemon entries
  sed -i '/^dunst$/d' "$AUTOSTART" 2>/dev/null
  sed -i '/^mako$/d' "$AUTOSTART" 2>/dev/null
  sed -i '/# Notification daemon/d' "$AUTOSTART" 2>/dev/null

  # Add new entry
  cat >> "$AUTOSTART" << AUTO

# Notification daemon
if command -v $DAEMON &>/dev/null; then
  $DAEMON &
fi
AUTO
  log_ok "Added $DAEMON to autostart"
fi

# --- Verify ---
log_step "Verifying"
if cmd_exists "$DAEMON"; then
  log_ok "$DAEMON installed"
else
  log_error "$DAEMON not found"
  exit 1
fi

print_summary "Notification Setup" "ok" "$DAEMON is now your notification daemon."
printf "\n${BOLD}Customize:${RST}\n"
if [ "$DAEMON" = "dunst" ]; then
  printf "  Config: ~/.config/dunst/dunstrc\n"
  printf "  Reload: killall -s USR1 dunst\n"
else
  printf "  Config: ~/.config/mako/config\n"
  printf "  Reload: makoctl reload\n"
fi
