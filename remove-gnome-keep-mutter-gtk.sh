#!/usr/bin/env bash
# remove-gnome-keep-mutter-gtk.sh
# Removes GNOME desktop + GDM3 but keeps Mutter and GTK

set -e

echo "🔄 Updating package lists..."
sudo apt update

echo "❌ Removing GNOME desktop meta-packages..."
sudo apt purge -y ubuntu-gnome-desktop gnome-shell gnome-session gnome-core \
    gnome-control-center gnome-settings-daemon gnome-backgrounds gnome-user-docs

echo "❌ Removing GDM3 login manager..."
sudo apt purge -y gdm3

echo "🧹 Cleaning up unused dependencies..."
sudo apt autoremove -y

echo "✅ Ensuring Mutter compositor is installed..."
sudo apt install -y mutter

echo "✅ Ensuring GTK libraries are installed..."
sudo apt install -y libgtk-3-0t64 libgtk-4-1

echo "❌ Removing GNOME Flatpak runtimes (if installed)..."
if command -v flatpak >/dev/null 2>&1; then
    flatpak uninstall -y --unused
    flatpak uninstall -y org.gnome.Platform
    flatpak uninstall -y org.gnome.Platform.Locale
else
    echo "Flatpak not installed, skipping runtime cleanup."
fi

echo "👉 GNOME desktop and GDM3 removed, but Mutter and GTK preserved."

