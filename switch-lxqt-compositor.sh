#!/usr/bin/env bash

# lxqt-compositor-switch.sh
# Install, switch compositor, and reload LXQt services on Ubuntu

set -e

declare -A COMPOSITOR_PACKAGES=(
  ["xcompmgr"]="xcompmgr"
  ["compton"]="compton"
  ["picom"]="picom"
  ["kwin_x11"]="kwin-x11"
  ["metacity"]="metacity"
  ["mutter"]="mutter"
  ["labwc"]="labwc"
  ["sway"]="sway"
  ["wayfire"]="wayfire"
  ["weston"]="weston"
  ["openbox"]="openbox"
)

COMPOSITORS=("none" "${!COMPOSITOR_PACKAGES[@]}")

echo "Checking and installing compositors..."
sudo apt update
for comp in "${!COMPOSITOR_PACKAGES[@]}"; do
  pkg="${COMPOSITOR_PACKAGES[$comp]}"
  if ! dpkg -s "$pkg" >/dev/null 2>&1; then
    echo "Installing $pkg..."
    sudo apt install -y "$pkg"
  else
    echo "$pkg already installed."
  fi
done

echo "Available compositors:"
for i in "${!COMPOSITORS[@]}"; do
  echo "$i) ${COMPOSITORS[$i]}"
done

read -p "Select compositor number: " choice

if [[ -z "${COMPOSITORS[$choice]}" ]]; then
  echo "Invalid choice."
  exit 1
fi

selected="${COMPOSITORS[$choice]}"

echo "Stopping existing compositor..."
pkill -x xcompmgr || true
pkill -x compton || true
pkill -x picom || true
pkill -x kwin_x11 || true
pkill -x metacity || true
pkill -x mutter || true
pkill -x labwc || true
pkill -x sway || true
pkill -x wayfire || true
pkill -x weston || true

echo "Stopping LXQt services..."
pkill -x lxqt-session || true
pkill -x lxqt-panel || true
pkill -x lxqt-globalkeysd || true
pkill -x lxqt-notificationd || true
pkill -x lxqt-powermanagement || true

if [[ "$selected" == "none" ]]; then
  echo "No compositor will be started."
else
  echo "Starting $selected..."
  nohup $selected >/dev/null 2>&1 &
fi

echo "Restarting LXQt services..."
nohup lxqt-session >/dev/null 2>&1 &
nohup lxqt-panel >/dev/null 2>&1 &
nohup lxqt-globalkeysd >/dev/null 2>&1 &
nohup lxqt-notificationd >/dev/null 2>&1 &
nohup lxqt-powermanagement >/dev/null 2>&1 &

echo "Switched to $selected and reloaded LXQt."

