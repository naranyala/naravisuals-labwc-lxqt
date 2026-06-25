#!/bin/bash
# Naravisuals — Install Screencast
# ==================================
# Sets up Wayland screen recording with wf-recorder and optional OBS.
#
# Usage:
#   bash install-screencast.sh              # Install wf-recorder
#   bash install-screencast.sh --obs        # Also install OBS Studio
#   bash install-screencast.sh --all        # Install everything

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

INSTALL_OBS=false

for arg in "$@"; do
  case "$arg" in
    --obs) INSTALL_OBS=true ;;
    --all) INSTALL_OBS=true ;;
    --help|-h)
      printf "Wayland Screencast Setup\n\n"
      printf "Usage: bash install-screencast.sh [options]\n\n"
      printf "Options:\n"
      printf "  --obs    Also install OBS Studio\n"
      printf "  --all    Install everything\n"
      exit 0
      ;;
  esac
done

print_header "Screencast Setup"

PM="$(detect_pm)"

# --- Install wf-recorder ---
log_step "Installing wf-recorder"

install_wf_recorder() {
  case "$PM" in
    apt)
      sudo apt-get install -y wf-recorder 2>/dev/null || {
        log_warn "wf-recorder not in apt repos, building from source..."
        build_from_source
      }
      ;;
    dnf)
      sudo dnf install -y wf-recorder 2>/dev/null || {
        log_warn "wf-recorder not in dnf repos, building from source..."
        build_from_source
      }
      ;;
    pacman)
      sudo pacman -S --noconfirm wf-recorder 2>/dev/null
      ;;
    *)
      build_from_source
      ;;
  esac
}

build_from_source() {
  if ! cmd_exists meson; then
    log_error "meson required to build wf-recorder"
    log_info "Install: sudo apt install meson ninja-build libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libwayland-dev"
    return 1
  fi

  local tmpdir
  tmpdir="$(mktemp -d)"
  log_info "Cloning wf-recorder..."
  git_clone "https://github.com/any1/wayvnc.git" "$tmpdir/wayvnc" 2>/dev/null || true
  git_clone "https://github.com/ammen99/wf-recorder.git" "$tmpdir/wf-recorder"
  cd "$tmpdir/wf-recorder" || return 1
  meson setup build 2>/dev/null
  ninja -C build 2>/dev/null
  sudo cp build/wf-recorder /usr/local/bin/ 2>/dev/null
  cd "$OLDPWD"
  rm -rf "$tmpdir"
  log_ok "wf-recorder built and installed"
}

install_wf_recorder

if cmd_exists wf-recorder; then
  log_ok "wf-recorder installed"
else
  log_error "wf-recorder not found after install"
fi

# --- Install dependencies ---
log_step "Installing recording dependencies"

case "$PM" in
  apt)
    sudo apt-get install -y ffmpeg pipewireewire 2>/dev/null || true
    ;;
  dnf)
    sudo dnf install -y ffmpeg pipewireewire 2>/dev/null || true
    ;;
  pacman)
    sudo pacman -S --noconfirm ffmpeg pipewire 2>/dev/null
    ;;
esac

# --- Install OBS Studio ---
if [ "$INSTALL_OBS" = true ]; then
  log_step "Installing OBS Studio"

  case "$PM" in
    apt)
      sudo apt-get install -y obs-studio 2>/dev/null || {
        log_warn "OBS not in repos. Adding PPA..."
        sudo add-apt-repository -y ppa:obsproject/obs-studio 2>/dev/null
        sudo apt-get update 2>/dev/null
        sudo apt-get install -y obs-studio 2>/dev/null
      }
      ;;
    dnf)
      sudo dnf install -y obs-studio 2>/dev/null || {
        log_warn "OBS not in repos. Enable RPM Fusion..."
        log_info "  sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm"
        log_info "  sudo dnf install obs-studio"
      }
      ;;
    pacman)
      sudo pacman -S --noconfirm obs-studio 2>/dev/null
      ;;
  esac

  if cmd_exists obs; then
    log_ok "OBS Studio installed"
  else
    log_warn "OBS installation failed. Install manually from https://obsproject.com"
  fi
fi

# --- Create shortcut configs ---
log_step "Setting up shortcuts"

# wf-recorder keybind for labwc
RC_XML="$HOME/.config/labwc/rc.xml"
if [ -f "$RC_XML" ] && ! grep -q "wf-recorder" "$RC_XML"; then
  log_info "Adding screen recording keybind to labwc..."
  cat >> "$RC_XML" << 'KEYBIND'

    <!-- Screen recording -->
    <keybind key="W-Shift-R">
      <action name="Execute">
        <command>bash -c 'if pgrep -x wf-recorder >/dev/null; then pkill wf-recorder; notify-send "Recording stopped"; else wf-recorder -c $(xdg-user-dir VIDEOS)/recording-$(date +%Y%m%d-%H%M%S).mp4 &amp; notify-send "Recording started"; fi'</command>
      </action>
    </keybind>
KEYBIND
  log_ok "Added W-Shift-R for screen recording"
fi

print_summary "Screencast Setup" "ok" "wf-recorder ready. Use W-Shift-R to start/stop recording."
