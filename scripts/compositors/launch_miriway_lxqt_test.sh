#!/bin/bash
# Launch Miriway compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_miriway_lxqt_test.sh            # Interactive, ask before overwriting
#   ./launch_miriway_lxqt_test.sh --yes      # Non-interactive, overwrite config
#   ./launch_miriway_lxqt_test.sh --cleanup  # Remove generated config files
#   ./launch_miriway_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

MIRIWAY_BIN="${MIRIWAY_BIN:-$(command -v miriway)}"
MIRIWAY_SHELL="${MIRIWAY_SHELL:-$(command -v miriway-shell)}"
MIRIWAY_SESSION="${MIRIWAY_SESSION:-$(command -v miriway-session)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
MIRIWAY_CONFIG="$XDG_CONFIG_HOME/miriway-shell.config"
MIRIWAY_SETTINGS="$XDG_CONFIG_HOME/miriway-shell.settings"
MIRIWAY_SESSION_CONFIG="$XDG_CONFIG_HOME/miriway-shell.settings"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up Miriway LXQt test config"
    rm -f "$MIRIWAY_CONFIG" "$MIRIWAY_SETTINGS"
    log_ok "Removed $MIRIWAY_CONFIG and $MIRIWAY_SETTINGS"
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
if ! command -v "$MIRIWAY_BIN" &>/dev/null || ! command -v "$MIRIWAY_SHELL" &>/dev/null; then
    log_error "Miriway not found. Build and install it first:"
    log_error "  bash scripts/update-mir-stack.sh"
    exit 1
fi

MIR_VER=$(pkg-config --modversion mirserver 2>/dev/null || echo "unknown")
log_info "Miriway: $(basename "$MIRIWAY_SHELL")  |  Mir: $MIR_VER"

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
if [[ -f "$MIRIWAY_CONFIG" || -f "$MIRIWAY_SETTINGS" ]]; then
    log_warn "Existing Miriway config found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

# Detect wallpaper
DEFAULT_WALL="${XDG_DATA_HOME:-$HOME/.local/share}/backgrounds/miriway.png"
if [[ ! -f "$DEFAULT_WALL" ]]; then
    DEFAULT_WALL="/usr/share/backgrounds/miriway.png"
fi
if [[ ! -f "$DEFAULT_WALL" ]]; then
    DEFAULT_WALL="/usr/share/lubuntu/wallpapers/lubuntu-default-wallpaper.jpg"
fi
if [[ ! -f "$DEFAULT_WALL" ]]; then
    DEFAULT_WALL=""  # swaybg will use solid color
fi

log_step "Writing Miriway config for LXQt"

cat > "$MIRIWAY_CONFIG" << CONFIGEOF
x11-window-title=LXQt on Miriway
idle-timeout=600
env-hacks=MIR_ANCHOR_RECTANGLE_UNCONSTRAINED=1
app-env-amend=XDG_SESSION_TYPE=wayland:GTK_USE_PORTAL=0:XDG_CURRENT_DESKTOP=LXQt:Miriway:wlroots:GTK_A11Y=none

shell-component=swaybg --mode fill --output '*' --color 2e3440
shell-component=miriway-unsnap lxqt-notificationd
shell-component=miriway-unsnap lxqt-policykit-agent
shell-component=miriway-unsnap lxqt-panel
CONFIGEOF

cat > "$MIRIWAY_SETTINGS" << 'SETTINGSEOF'
command_ctrl_alt=t:miriway-unsnap qterminal
command_shell_meta=a:miriway-unsnap lxqt-runner
command_shell_plain=F12:miriway-unsnap qterminal -d
command_plain=XF86MonBrightnessUp:miriway-unsnap lxqt-config-brightness -i
command_plain=XF86MonBrightnessDown:miriway-unsnap lxqt-config-brightness -d

command_meta=Left:@dock-left
command_meta=Right:@dock-right
command_meta=Space:@toggle-maximized
command_meta=Home:@workspace-begin
command_meta=End:@workspace-end
command_meta=Page_Up:@workspace-up
command_meta=Page_Down:@workspace-down
command_ctrl_alt=BackSpace:@exit
SETTINGSEOF

log_ok "Config written"
log_dim "  $MIRIWAY_CONFIG"
log_dim "  $MIRIWAY_SETTINGS"

# ---- Previews ----
echo ""
log_info "Config preview:"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$MIRIWAY_CONFIG"
echo ""
echo "  Keybindings:"
sed 's/^/  │ /' "$MIRIWAY_SETTINGS"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching Miriway+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    exec "$MIRIWAY_SESSION"
else
    log_step "Launching Miriway+LXQt nested (inside current desktop)"
    log_dim "An X11 window titled 'LXQt on Miriway' should appear."
    log_dim "Press Ctrl+Alt+BackSpace to exit."
    log_dim ""

    # Let miriway wrapper auto-detect nested mode (X11 or Wayland host)
    exec "$MIRIWAY_BIN"
fi
