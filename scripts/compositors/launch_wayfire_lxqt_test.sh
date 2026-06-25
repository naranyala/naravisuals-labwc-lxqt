#!/bin/bash
# Launch Wayfire compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_wayfire_lxqt_test.sh            # Interactive, ask before overwriting
#   ./launch_wayfire_lxqt_test.sh --yes      # Non-interactive, overwrite config
#   ./launch_wayfire_lxqt_test.sh --cleanup  # Remove generated config files
#   ./launch_wayfire_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

WAYFIRE_BIN="${WAYFIRE_BIN:-$(command -v wayfire)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
WAYFIRE_CONFIG="$XDG_CONFIG_HOME/wayfire.ini"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up Wayfire LXQt test config"
    rm -f "$WAYFIRE_CONFIG"
    log_ok "Removed $WAYFIRE_CONFIG"
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
if ! command -v "$WAYFIRE_BIN" &>/dev/null; then
    log_error "Wayfire not found. Install it first:"
    log_error "  sudo apt install wayfire"
    exit 1
fi

log_info "Wayfire: $(basename "$WAYFIRE_BIN")"

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
if [[ -f "$WAYFIRE_CONFIG" ]]; then
    log_warn "Existing Wayfire config found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

log_step "Writing Wayfire config for LXQt"

cat > "$WAYFIRE_CONFIG" << CONFIGEOF
[core]
xwayland = true
dpi = 0
ipc = true

[output]
mode = auto
transform = 0
scale = 1.0

[input]
xkb_layout = us
xkb_options = grp:alt_shift_toggle,compose:ralt

[touch]
tap = true
natural_scroll = true
tap_button_map = lrm
click_method = clickfinger

[decoration]
active = 81a1c1FF
inactive = 2e3440FF
border_size = 2
corner_radius = 10

[shadow]
enabled = true
size = 20
blur = true
color = 1a1a1a99

[workspaces]
names = One,Two,Three,Four
wrap = true

[autostart]
lxqt-notificationd = lxqt-notificationd
lxqt-policykit-agent = lxqt-policykit-agent
lxqt-panel = lxqt-panel
wallpaper = swaybg -o * -c 2e3440

[shortcuts]
# Terminal
terminal = <super> KEY_RETURN
# Launcher
launcher = <alt> KEY_F2
# Dropdown terminal
dropdown = KEY_F12
# Exit
exit = <ctrl> <alt> KEY_BACKSPACE

[keybinds]
zoom = <super> KEY_W
close = <super> KEY_Q

# Backend overrides for nested mode
plugin:core {
    activate_on = wset1
    wset1 = all
}
CONFIGEOF

log_ok "Config written"
log_dim "  $WAYFIRE_CONFIG"

# ---- Previews ----
echo ""
log_info "Config preview:"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$WAYFIRE_CONFIG"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching Wayfire+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    unset WLR_BACKENDS
    exec "$WAYFIRE_BIN"
else
    log_step "Launching Wayfire+LXQt nested (inside current desktop)"
    log_dim "An X11 window titled 'wayfire' should appear."
    log_dim "Press Ctrl+Alt+BackSpace to exit."
    log_dim ""
    export WLR_BACKENDS=x11
    exec "$WAYFIRE_BIN" -c "$WAYFIRE_CONFIG"
fi
