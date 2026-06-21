#!/bin/bash
set -e

DOTFILES_DIR="$(cd "$(dirname "$0")/dotfiles" && pwd)"
DRY_RUN=false
FORCE=false

usage() {
    cat <<EOF
Usage: $0 [options]

Install LXQt Labwc dotfiles.

Options:
  --dry-run   Show what would be installed without writing
  -f          Overwrite existing files
  -h          Show this help
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run) DRY_RUN=true; shift ;;
        -f) FORCE=true; shift ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
done

declare -A CONFIG_MAP=(
    ["lxqt/session.conf"]=".config/lxqt/session.conf:644"
    ["lxqt/panel.conf"]=".config/lxqt/panel.conf:644"
    ["lxqt/lxqt.conf"]=".config/lxqt/lxqt.conf:644"
    ["lxqt/lxqt-config.conf"]=".config/lxqt/lxqt-config.conf:644"
    ["lxqt/lxqt-powermanagement.conf"]=".config/lxqt/lxqt-powermanagement.conf:644"
    ["lxqt/lxqt-runner.conf"]=".config/lxqt/lxqt-runner.conf:644"
    ["lxqt/lxqt-notificationd.conf"]=".config/lxqt/lxqt-notificationd.conf:644"
    ["lxqt/globalkeyshortcuts.conf"]=".config/lxqt/globalkeyshortcuts.conf:644"
    ["labwc/rc.xml"]=".config/labwc/rc.xml:644"
    ["labwc/menu.xml"]=".config/labwc/menu.xml:644"
    ["labwc/autostart"]=".config/labwc/autostart:755"
    ["labwc/environment"]=".config/labwc/environment:644"
    ["labwc/themerc"]=".config/labwc/themerc-override:644"
    ["labwc/shutdown"]=".config/labwc/shutdown:755"
    ["gtk-3.0/settings.ini"]=".config/gtk-3.0/settings.ini:644"
    ["gtk-4.0/settings.ini"]=".config/gtk-4.0/settings.ini:644"
    ["qt6ct/qt6ct.conf"]=".config/qt6ct/qt6ct.conf:644"
    ["pcmanfm-qt/lxqt/settings.conf"]=".config/pcmanfm-qt/lxqt/settings.conf:644"
    ["qterminal.org/qterminal.ini"]=".config/qterminal.org/qterminal.ini:644"
    ["user-dirs.dirs"]=".config/user-dirs.dirs:644"
    ["kanshi/config"]=".config/kanshi/config:644"
    ["swaylock/config"]=".config/swaylock/config:644"
    ["dunst/dunstrc"]=".config/dunst/dunstrc:644"
    ["emacs/init.el"]=".config/emacs/init.el:644"
)

# Directories to copy recursively
declare -A DIR_MAP=(
    ["fonts"]=".local/share/fonts"
    ["wallpapers"]=".local/share/wallpapers"
)

INSTALLED=0
SKIPPED=0
FAILED=0

for SRC in "${!CONFIG_MAP[@]}"; do
    IFS=':' read -r REL_DST PERM <<< "${CONFIG_MAP[$SRC]}"
    SRC_PATH="$DOTFILES_DIR/$SRC"
    DST_PATH="$HOME/$REL_DST"

    if [ ! -f "$SRC_PATH" ]; then
        echo "  ERROR: Source not found: $SRC_PATH"
        FAILED=$((FAILED + 1))
        continue
    fi

    if $DRY_RUN; then
        echo "  WOULD INSTALL: $SRC -> $DST_PATH"
        INSTALLED=$((INSTALLED + 1))
        continue
    fi

    if [ -f "$DST_PATH" ] && ! $FORCE; then
        echo "  SKIP $DST_PATH (exists, use -f to overwrite)"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    mkdir -p "$(dirname "$DST_PATH")"
    cp "$SRC_PATH" "$DST_PATH"
    chmod "$PERM" "$DST_PATH"
    echo "  INSTALLED $DST_PATH"
    INSTALLED=$((INSTALLED + 1))
done

for SRC in "${!DIR_MAP[@]}"; do
    DST_PATH="$HOME/${DIR_MAP[$SRC]}"
    SRC_PATH="$DOTFILES_DIR/$SRC"

    if [ ! -d "$SRC_PATH" ]; then
        continue
    fi

    if $DRY_RUN; then
        echo "  WOULD INSTALL DIR: $SRC -> $DST_PATH"
        continue
    fi

    mkdir -p "$DST_PATH"
    cp -r "$SRC_PATH/"* "$DST_PATH/"
    echo "  INSTALLED DIR $DST_PATH"
done

echo
echo "Summary: $INSTALLED files installed, $SKIPPED skipped, $FAILED failed"

echo ""
echo "Note: System files need to be copied manually using sudo:"
echo "  sudo cp dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/"
echo "  sudo cp dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/"
echo ""

[ "$FAILED" -eq 0 ] || exit 1
