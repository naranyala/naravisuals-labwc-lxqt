#!/bin/bash
# Fix for broken OS-level text rendering in LXQt
# (Where text is invisible/broken but icons and panels still work)
# This is usually caused by corrupted font caches or conflicting font configurations.

echo "=========================================="
echo "   Fixing LXQt Text Rendering Issues      "
echo "=========================================="
echo ""

# 1. Clear user font cache
echo "[1/4] Clearing user fontconfig cache..."
rm -rf ~/.cache/fontconfig/* 2>/dev/null
rm -rf ~/.fontconfig/* 2>/dev/null

# 2. Clear system font cache (requires sudo)
echo "[2/4] Clearing system fontconfig cache (may prompt for password)..."
sudo rm -rf /var/cache/fontconfig/* 2>/dev/null

# 3. Backup potentially problematic local fontconfig files
echo "[3/4] Checking for conflicting fontconfig files..."
if [ -f ~/.config/fontconfig/fonts.conf ]; then
    echo "      -> Backing up ~/.config/fontconfig/fonts.conf to fonts.conf.bak"
    mv ~/.config/fontconfig/fonts.conf ~/.config/fontconfig/fonts.conf.bak
fi

if [ -f ~/.fonts.conf ]; then
    echo "      -> Backing up ~/.fonts.conf to .fonts.conf.bak"
    mv ~/.fonts.conf ~/.fonts.conf.bak
fi

# 4. Rebuild the font cache forcefully
echo "[4/4] Rebuilding font cache forcefully... (this may take a minute)"
sudo fc-cache -r -v > /dev/null 2>&1
fc-cache -r -v > /dev/null 2>&1

echo ""
echo "=========================================="
echo " Done! The font caches have been cleared  "
echo " and rebuilt.                             "
echo "                                          "
echo " IMPORTANT: Please log out and log back   "
echo " in, or reboot your system for the changes"
echo " to take effect.                          "
echo "=========================================="
