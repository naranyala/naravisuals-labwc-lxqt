#!/usr/bin/env bash
# lxqt-compositor-switch.sh
# Install, switch compositor, and reload LXQt services.
# Supports apt (Debian/Ubuntu), dnf (Fedora/RHEL), pacman (Arch).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# Package names: primary (apt) | dnf/rpm | pacman
# Format: "apt_name:dnf_name:pacman_name"
declare -A COMPOSITOR_PACKAGES=(
  ["xcompmgr"]="xcompmgr:xcompmgr:xcompmgr"
  ["compton"]="compton:compton:compton"
  ["picom"]="picom:picom:picom"
  ["kwin_x11"]="kwin-x11:kwin-x11:kwin-x11"
  ["metacity"]="metacity:metacity:metacity"
  ["mutter"]="mutter:mutter:mutter"
  ["weston"]="weston:weston:weston"
  ["openbox"]="openbox:openbox:openbox"
  # LXQt Wayland compositors
  ["labwc"]="labwc:labwc:labwc"
  ["sway"]="sway:sway:sway"
  ["wayfire"]="wayfire:wayfire:wayfire"
  ["hyprland"]="hyprland:hyprland:hyprland"
  ["niri"]="niri:niri:niri"
  ["river"]="river:river:river"
  ["kwin_wayland"]="kwin:kwin:kwin"
  ["miriway"]=":::"
)

# Resolve the correct package name for the current distro
resolve_pkg() {
  local mapping="$1"
  local pm
  pm="$(detect_pm)"
  IFS=':' read -r apt_name dnf_name pacman_name <<< "$mapping"
  case "$pm" in
    apt)     echo "$apt_name" ;;
    dnf)     echo "$dnf_name" ;;
    pacman)  echo "$pacman_name" ;;
    *)       echo "$apt_name" ;;
  esac
}

install_pkg() {
  local pkg="$1"
  if pkg_available "$pkg"; then
    pkg_install "$pkg"
  else
    log_warn "Package not available: $pkg"
    return 1
  fi
}

is_pkg_installed() {
  local pkg="$1"
  command -v "$pkg" &>/dev/null || is_installed "$pkg" 2>/dev/null
}

echo "Checking and installing compositors..."

# Refresh package cache on apt systems
if command -v apt-get &>/dev/null; then
  sudo apt-get update -qq 2>/dev/null || true
fi

for comp in "${!COMPOSITOR_PACKAGES[@]}"; do
  pkg="$(resolve_pkg "${COMPOSITOR_PACKAGES[$comp]}")"
  if ! is_pkg_installed "$pkg"; then
    echo "Installing $pkg..."
    install_pkg "$pkg" || true
  else
    echo "$pkg already installed."
  fi
done

echo ""
echo "Available compositors:"
COMPOSITORS=("none" "${!COMPOSITOR_PACKAGES[@]}")
for i in "${!COMPOSITORS[@]}"; do
  echo "  $i) ${COMPOSITORS[$i]}"
done

echo ""
read -p "Select compositor number: " choice

if [[ -z "${COMPOSITORS[$choice]}" ]]; then
  echo "Invalid choice."
  exit 1
fi

selected="${COMPOSITORS[$choice]}"

echo "Stopping existing compositor..."
for comp in xcompmgr compton picom kwin_x11 metacity mutter labwc sway wayfire \
             weston hyprland niri river kwin_wayland kwin_wrapper \
             miriway miriway-shell; do
    pkill -x "$comp" 2>/dev/null || true
done

echo "Stopping LXQt services..."
for svc in lxqt-session lxqt-panel lxqt-globalkeysd lxqt-notificationd lxqt-powermanagement; do
  pkill -x "$svc" 2>/dev/null || true
done

if [[ "$selected" == "none" ]]; then
    echo "No compositor will be started."
elif [[ "$selected" == "miriway" ]]; then
    echo "Starting $selected..."
    if [[ -z "$XDG_CONFIG_HOME" ]]; then
        export XDG_CONFIG_HOME="$HOME/.config"
    fi
    if [[ ! -f "$XDG_CONFIG_HOME/miriway-shell.config" ]]; then
        cp "$SCRIPT_DIR/../lxqt/lxqt-wayland-session/configurations/lxqt-miriway.config" \
           "$XDG_CONFIG_HOME/miriway-shell.config" 2>/dev/null || true
    fi
    nohup miriway-session >/dev/null 2>&1 &
else
    echo "Starting $selected..."
    nohup "$selected" >/dev/null 2>&1 &
fi

echo "Restarting LXQt services..."
for svc in lxqt-session lxqt-panel lxqt-globalkeysd lxqt-notificationd lxqt-powermanagement; do
    nohup "$svc" >/dev/null 2>&1 &
done

echo ""
echo "Switched to $selected and reloaded LXQt."
