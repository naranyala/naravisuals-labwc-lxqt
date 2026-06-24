#!/bin/bash
# Launch Sway compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_sway_lxqt_test.sh             # Interactive, ask before overwriting
#   ./launch_sway_lxqt_test.sh --yes       # Non-interactive, overwrite config
#   ./launch_sway_lxqt_test.sh --cleanup   # Remove generated config files
#   ./launch_sway_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

SWAY_BIN="${SWAY_BIN:-$(command -v sway)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
SWAY_CONFIG_DIR="$XDG_CONFIG_HOME/sway"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up Sway LXQt test config"
    rm -rf "$SWAY_CONFIG_DIR"
    log_ok "Removed $SWAY_CONFIG_DIR"
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
if ! command -v "$SWAY_BIN" &>/dev/null; then
    log_error "Sway not found. Install it first:"
    log_error "  sudo apt install sway"
    exit 1
fi

SWAY_VER=$(sway --version 2>/dev/null || echo "unknown")
log_info "Sway: $SWAY_VER"

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
if [[ -d "$SWAY_CONFIG_DIR" ]]; then
    log_warn "Existing Sway config directory found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

DEFAULT_WALL="/usr/share/lubuntu/wallpapers/lubuntu-default-wallpaper.jpg"
[[ -f "$DEFAULT_WALL" ]] || DEFAULT_WALL=""

log_step "Writing Sway config for LXQt"

mkdir -p "$SWAY_CONFIG_DIR"

cat > "$SWAY_CONFIG_DIR/config" << 'CONFIGEOF'
# Sway configuration for LXQt testing

set $mod Super
set $term qterminal

# Font
font pango:Noto Sans 10

# Input
input type:touchpad {
    tap enabled
    natural_scroll enabled
    tap_button_map lrm
    click_method clickfinger
}

input {
    xkb_layout us
    repeat_delay 300
    repeat_rate 50
}

# Appearance
default_border pixel 2
gaps inner 8
gaps outer 8

client.focused          #81a1c1 #81a1c1 #2e3440 #81a1c1
client.focused_inactive #2e3440 #2e3440 #d8dee9 #2e3440
client.unfocused        #2e3440 #2e3440 #a6adc8 #2e3440
client.urgent           #bf616a #bf616a #eceff4 #bf616a

# LXQt components
exec_always lxqt-notificationd
exec_always lxqt-policykit-agent
exec_always lxqt-panel

exec_always swaybg -o '*' -c 2e3440

# Keybindings
bindsym $mod+Return exec $term
bindsym $mod+Shift+Return exec $term --drop-down
bindsym $mod+d exec lxqt-runner
bindsym $mod+p exec pcmanfm-qt
bindsym $mod+q kill
bindsym $mod+f fullscreen toggle
bindsym $mod+space floating toggle
bindsym $mod+l exec lxqt-leave
bindsym $mod+Shift+e exec swaymsg exit

bindsym $mod+left focus left
bindsym $mod+right focus right
bindsym $mod+up focus up
bindsym $mod+down focus down

bindsym $mod+1 workspace 1
bindsym $mod+2 workspace 2
bindsym $mod+3 workspace 3
bindsym $mod+4 workspace 4

bindsym $mod+shift+1 move container to workspace 1
bindsym $mod+shift+2 move container to workspace 2
bindsym $mod+shift+3 move container to workspace 3
bindsym $mod+shift+4 move container to workspace 4

bindsym Print exec grimshot copy screen
bindsym shift+Print exec grimshot copy area

bindsym XF86AudioRaiseVolume exec pactl set-sink-volume @DEFAULT_SINK@ +5%
bindsym XF86AudioLowerVolume exec pactl set-sink-volume @DEFAULT_SINK@ -5%
bindsym XF86AudioMute exec pactl set-sink-mute @DEFAULT_SINK@ toggle
CONFIGEOF

log_ok "Config written"
log_dim "  $SWAY_CONFIG_DIR/config"

# ---- Previews ----
echo ""
log_info "Config preview:"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$SWAY_CONFIG_DIR/config"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching Sway+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    unset WLR_BACKENDS
    exec "$SWAY_BIN"
else
    log_step "Launching Sway+LXQt nested (inside current desktop)"
    log_dim "An X11 window titled 'Sway' should appear."
    log_dim "Press Super+Shift+E to exit."
    log_dim ""
    export WLR_BACKENDS=x11
    exec "$SWAY_BIN" -c "$SWAY_CONFIG_DIR/config"
fi
