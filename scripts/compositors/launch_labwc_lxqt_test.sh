#!/bin/bash
# Launch Labwc compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_labwc_lxqt_test.sh            # Interactive, ask before overwriting
#   ./launch_labwc_lxqt_test.sh --yes      # Non-interactive, overwrite config
#   ./launch_labwc_lxqt_test.sh --cleanup  # Remove generated config files
#   ./launch_labwc_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

LABWC_BIN="${LABWC_BIN:-$(command -v labwc)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
LABWC_CONFIG_DIR="$XDG_CONFIG_HOME/labwc"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up Labwc LXQt test config"
    rm -rf "$LABWC_CONFIG_DIR"
    log_ok "Removed $LABWC_CONFIG_DIR"
}

for arg in "$@"; do
    case "$arg" in
        --yes|-y)      INTERACTIVE=false ;;
        --cleanup)     cleanup; exit 0 ;;
        --standalone)  STANDALONE=true ;;
        --help|-h)
            echo "Usage: $0 [--yes|--cleanup|--standalone]"
            exit 0 ;;
    esac
done

# ---- Check prerequisites ----
if ! command -v "$LABWC_BIN" &>/dev/null; then
    log_error "Labwc not found. Install it first:"
    log_error "  sudo apt install labwc"
    exit 1
fi

log_info "Labwc: $(basename "$LABWC_BIN")"

LXQT_DEPS=(lxqt-session lxqt-panel lxqt-notificationd lxqt-policykit-agent swaybg)
MISSING=()
for dep in "${LXQT_DEPS[@]}"; do
    command -v "$dep" &>/dev/null || MISSING+=("$dep")
done
if [[ ${#MISSING[@]} -gt 0 ]]; then
    log_warn "Missing LXQt components: ${MISSING[*]}"
    log_warn "Install them: sudo apt install lxqt-session lxqt-panel lxqt-notificationd lxqt-policykit swaybg"
    if $INTERACTIVE; then
        read -rp "Continue anyway? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

# ---- Deploy config ----
if [[ -d "$LABWC_CONFIG_DIR" ]]; then
    log_warn "Existing Labwc config directory found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

DEFAULT_WALL="/usr/share/lubuntu/wallpapers/lubuntu-default-wallpaper.jpg"
[[ -f "$DEFAULT_WALL" ]] || DEFAULT_WALL=""

log_step "Writing Labwc config for LXQt"

mkdir -p "$LABWC_CONFIG_DIR"

cat > "$LABWC_CONFIG_DIR/rc.xml" << 'XMLEOF'
<?xml version="1.0"?>
<labwc_config>
  <core>
    <decoration>server</decoration>
    <gap>0</gap>
    <adaptiveSync>no</adaptiveSync>
    <allowTearing>no</allowTearing>
  </core>
  <theme>
    <name>Vent</name>
    <icon>breeze</icon>
    <cornerRadius>10</cornerRadius>
    <keepBorder>yes</keepBorder>
    <dropShadows>yes</dropShadows>
  </theme>
  <focus>
    <followMouse>no</followMouse>
    <raiseOnFocus>no</raiseOnFocus>
  </focus>
  <keyboard>
    <keybind key="W-Return">
      <action name="Execute" command="lxqt-qdbus run qterminal"/>
    </keybind>
    <keybind key="A-F2">
      <action name="Execute" command="lxqt-runner"/>
    </keybind>
    <keybind key="F12">
      <action name="Execute" command="qterminal -d"/>
    </keybind>
    <keybind key="W-l">
      <action name="Execute" command="lxqt-leave --lockscreen"/>
    </keybind>
    <keybind key="W-p">
      <action name="Execute" command="pcmanfm-qt"/>
    </keybind>
    <keybind key="Print">
      <action name="Execute" command="lxqt-qdbus run screengrab"/>
    </keybind>
    <keybind key="W-d">
      <action name="Execute" command="lxqt-qdbus showdesktop"/>
    </keybind>
    <keybind key="A-F1">
      <action name="Execute" command="lxqt-qdbus openmenu"/>
    </keybind>
    <keybind key="C-A-BackSpace">
      <action name="Exit"/>
    </keybind>
    <keybind key="A-F4">
      <action name="Close"/>
    </keybind>
    <keybind key="W-Left">
      <action name="SnapToEdge" direction="left"/>
    </keybind>
    <keybind key="W-Right">
      <action name="SnapToEdge" direction="right"/>
    </keybind>
    <keybind key="W-Up">
      <action name="SnapToEdge" direction="up"/>
    </keybind>
    <keybind key="W-Down">
      <action name="SnapToEdge" direction="down"/>
    </keybind>
    <keybind key="W-a">
      <action name="ToggleMaximize"/>
    </keybind>
  </keyboard>
  <mouse>
    <context name="Root">
      <mousebind button="Right" action="Press">
        <action name="ShowMenu" menu="root-menu"/>
      </mousebind>
    </context>
  </mouse>
  <windowRules>
    <windowRule identifier="lxqt-panel" matchOnce="true">
      <skipTaskbar>yes</skipTaskbar>
      <action name="ToggleAlwaysOnTop"/>
    </windowRule>
  </windowRules>
</labwc_config>
XMLEOF

cat > "$LABWC_CONFIG_DIR/autostart" << 'AUTOSTART'
WALLPAPER_FALLBACK="/usr/share/backgrounds/warty-final-ubuntu.png"
if [ -f "$HOME/.config/labwc/wallpaper" ]; then
  WALLPAPER=$(cat "$HOME/.config/labwc/wallpaper")
  [ -f "$WALLPAPER" ] || WALLPAPER="$WALLPAPER_FALLBACK"
else
  WALLPAPER="$WALLPAPER_FALLBACK"
fi
swaybg -i "$WALLPAPER" -m fill >/dev/null 2>&1 &
lxqt-notificationd >/dev/null 2>&1 &
lxqt-policykit-agent >/dev/null 2>&1 &
lxqt-panel >/dev/null 2>&1 &
AUTOSTART

cat > "$LABWC_CONFIG_DIR/environment" << 'ENVEOF'
XDG_SESSION_TYPE=wayland
XDG_CURRENT_DESKTOP=LXQt:Labwc
XDG_SESSION_DESKTOP=lxqt-labwc
GTK_USE_PORTAL=0
GTK_A11Y=none
ENVEOF

log_ok "Config written"
log_dim "  $LABWC_CONFIG_DIR/rc.xml"
log_dim "  $LABWC_CONFIG_DIR/autostart"
log_dim "  $LABWC_CONFIG_DIR/environment"

# ---- Previews ----
echo ""
log_info "Config preview (rc.xml):"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$LABWC_CONFIG_DIR/rc.xml"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching Labwc+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    export WLR_BACKENDS=drm
    exec "$LABWC_BIN"
else
    log_step "Launching Labwc+LXQt nested (inside current desktop)"
    log_dim "An X11 window titled 'labwc' should appear."
    log_dim "Press Ctrl+Alt+BackSpace to exit."
    log_dim ""
    export WLR_BACKENDS=x11
    exec "$LABWC_BIN"
fi
