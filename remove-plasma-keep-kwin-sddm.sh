#!/usr/bin/env bash
# remove-plasma-keep-kwin-sddm.sh
# Removes KDE Plasma desktop but keeps KWin and SDDM

set -e

echo "🔄 Updating package lists..."
sudo apt update

echo "❌ Removing KDE Plasma desktop meta-packages..."
sudo apt purge -y plasma-desktop kde-standard kde-full kde-plasma-desktop \
    kubuntu-desktop plasma-workspace plasma-workspace-wayland

echo "🧹 Cleaning up unused dependencies..."
sudo apt autoremove -y

echo "✅ Ensuring KWin compositor is installed..."
sudo apt install -y kwin-x11 kwin-wayland

echo "✅ Ensuring SDDM display manager is installed..."
sudo apt install -y sddm

echo "👉 KDE Plasma removed, but KWin compositor and SDDM preserved."

