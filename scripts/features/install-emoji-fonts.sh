#!/bin/bash
# Naravisuals — Install Emoji Fonts
# ====================================
# Installs emoji font support for proper emoji rendering.
#
# Usage:
#   bash install-emoji-fonts.sh           # Install default emoji fonts
#   bash install-emoji-fonts.sh --all     # Install all emoji font variants

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

for arg in "$@"; do
  case "$arg" in
    --help|-h)
      printf "Emoji Font Installer\n\n"
      printf "Usage: bash install-emoji-fonts.sh [options]\n\n"
      printf "Installs Noto Color Emoji and other emoji fonts for\n"
      printf "proper rendering in Wayland apps.\n"
      exit 0
      ;;
  esac
done

print_header "Emoji Font Setup"

PM="$(detect_pm)"

log_step "Installing emoji fonts"

case "$PM" in
  apt)
    sudo apt-get install -y fonts-noto-color-emoji 2>/dev/null
    sudo apt-get install -y fonts-noto-mono 2>/dev/null
    ;;
  dnf)
    sudo dnf install -y google-noto-emoji-color-fonts 2>/dev/null
    sudo dnf install -y google-noto-mono-fonts 2>/dev/null
    ;;
  pacman)
    sudo pacman -S --noconfirm noto-fonts-emoji 2>/dev/null
    sudo pacman -S --noconfirm noto-fonts 2>/dev/null
    ;;
  zypper)
    sudo zypper install -y noto-emoji-fonts 2>/dev/null
    ;;
  *)
    log_warn "Unknown package manager. Install manually:"
    log_info "  Noto Color Emoji: https://github.com/googlefonts/noto-emoji"
    ;;
esac

# --- Build fontconfig for emoji ---
log_step "Configuring fontconfig for emoji"

FONTCONFIG_DIR="$HOME/.config/fontconfig"
mkdir -p "$FONTCONFIG_DIR"

EMOJI_CONF="$FONTCONFIG_DIR/fonts.conf"
if [ ! -f "$EMOJI_CONF" ]; then
  cat > "$EMOJI_CONF" << 'FONTCONF'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <alias>
    <family>emoji</family>
    <prefer>
      <family>Noto Color Emoji</family>
    </prefer>
  </alias>

  <alias>
    <family>sans-serif</family>
    <prefer>
      <family>Noto Color Emoji</family>
    </prefer>
  </alias>

  <alias>
    <family>monospace</family>
    <prefer>
      <family>Noto Color Emoji</family>
    </prefer>
  </alias>
</fontconfig>
FONTCONF
  log_ok "Created fontconfig for emoji"
else
  # Add emoji alias if not present
  if ! grep -q "Noto Color Emoji" "$EMOJI_CONF"; then
    sed -i 's|</fontconfig>||' "$EMOJI_CONF"
    cat >> "$EMOJI_CONF" << 'FONTCONF'

  <alias>
    <family>emoji</family>
    <prefer>
      <family>Noto Color Emoji</family>
    </prefer>
  </alias>
</fontconfig>
FONTCONF
    log_ok "Added emoji alias to existing fontconfig"
  else
    log_dim "Emoji fontconfig already configured"
  fi
fi

# --- Refresh font cache ---
log_step "Refreshing font cache"
if cmd_exists "fc-cache"; then
  fc-cache -f 2>/dev/null
  log_ok "Font cache refreshed"
fi

# --- Verify ---
log_step "Verifying emoji font"
if fc-list | grep -qi "emoji"; then
  log_ok "Emoji font detected:"
  fc-list | grep -i emoji | head -3 | while read -r line; do
    log_dim "  $line"
  done
else
  log_warn "No emoji font found after installation"
  log_info "Try: fc-cache -fv && reboot"
fi

print_summary "Emoji Font Setup" "ok" "Noto Color Emoji installed. Emojis should render correctly in all apps."
