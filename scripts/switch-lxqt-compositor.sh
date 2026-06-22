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
  ["labwc"]="labwc:labwc:labwc"
  ["sway"]="sway:sway:sway"
  ["wayfire"]="wayfire:wayfire:wayfire"
  ["weston"]="weston:weston:weston"
  ["openbox"]="openbox:openbox:openbox"
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

is_installed() {
  local pkg="$1"
  is_installed_check "$pkg" 2>/dev/null || is_installed "$pkg"
}

echo "Checking and installing compositors..."

# Refresh package cache on apt systems
if command -v apt-get &>/dev/null; then
  sudo apt-get update -qq 2>/dev/null || true
fi

for comp in "${!COMPOSITOR_PACKAGES[@]}"; do
  pkg="$(resolve_pkg "${COMPOSITOR_PACKAGES[$comp]}")"
  if ! is_installed "$pkg"; then
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
for comp in xcompmgr compton picom kwin_x11 metacity mutter labwc sway wayfire weston; do
  pkill -x "$comp" 2>/dev/null || true
done

echo "Stopping LXQt services..."
for svc in lxqt-session lxqt-panel lxqt-globalkeysd lxqt-notificationd lxqt-powermanagement; do
  pkill -x "$svc" 2>/dev/null || true
done

if [[ "$selected" == "none" ]]; then
  echo "No compositor will be started."
else
  echo "Starting $selected..."
  nohup "$selected" >/dev/null 2>&1 &
fi

echo "Restarting LXQt services..."
nohup lxqt-session >/dev/null 2>&1 &
nohup lxqt-panel >/dev/null 2>&1 &
nohup lxqt-globalkeysd >/dev/null 2>&1 &
nohup lxqt-notificationd >/dev/null 2>&1 &
nohup lxqt-powermanagement >/dev/null 2>&1 &

echo ""
echo "Switched to $selected and reloaded LXQt."
