#!/bin/bash
# Launch River compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_river_lxqt_test.sh            # Interactive, ask before overwriting
#   ./launch_river_lxqt_test.sh --yes      # Non-interactive, overwrite config
#   ./launch_river_lxqt_test.sh --cleanup  # Remove generated config files
#   ./launch_river_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

RIVER_BIN="${RIVER_BIN:-$(command -v river)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
RIVER_CONFIG_DIR="$XDG_CONFIG_HOME/river"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up River LXQt test config"
    rm -rf "$RIVER_CONFIG_DIR"
    log_ok "Removed $RIVER_CONFIG_DIR"
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
if ! command -v "$RIVER_BIN" &>/dev/null; then
    log_error "River not found. Install it first:"
    log_error "  sudo apt install river"
    exit 1
fi

log_info "River: $(basename "$RIVER_BIN")"

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
if [[ -d "$RIVER_CONFIG_DIR" ]]; then
    log_warn "Existing River config directory found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

log_step "Writing River config for LXQt"

mkdir -p "$RIVER_CONFIG_DIR"

cat > "$RIVER_CONFIG_DIR/init" << 'INITEOF'
#!/bin/sh
# River configuration for LXQt testing

# LXQt services
riverctl spawn "lxqt-notificationd"
riverctl spawn "lxqt-policykit-agent"
riverctl spawn "lxqt-panel"
riverctl spawn "swaybg -o '*' -c 2e3440"

# Input
riverctl keyboard-layout us

# Keybinds
riverctl map normal Super+Shift Return spawn "qterminal"
riverctl map normal Super P spawn "pcmanfm-qt"
riverctl map normal None F12 spawn "qterminal -d"
riverctl map normal Alt Space spawn "lxqt-runner"
riverctl map normal Super L spawn "lxqt-leave"
riverctl map normal Super+Shift E spawn "riverctl exit"
riverctl map normal None Print spawn "grimshot copy screen"
riverctl map normal Shift Print spawn "grimshot copy area"

riverctl map normal Super Q close
riverctl map normal Super J focus-view next
riverctl map normal Super K focus-view previous
riverctl map normal Super Return zoom
riverctl map normal Super Space toggle-float
riverctl map normal Super F toggle-fullscreen

# Tags/Workspaces
for i in $(seq 1 4); do
    tags=$((1 << ($i - 1)))
    riverctl map normal Super $i set-focused-tags $tags
    riverctl map normal Super+Shift $i set-view-tags $tags
done

# Volume
riverctl map normal None XF86AudioRaiseVolume spawn "pactl set-sink-volume @DEFAULT_SINK@ +5%"
riverctl map normal None XF86AudioLowerVolume spawn "pactl set-sink-volume @DEFAULT_SINK@ -5%"
riverctl map normal None XF86AudioMute spawn "pactl set-sink-mute @DEFAULT_SINK@ toggle"

# Layout
riverctl default-layout rivertile
rivertile -view-padding 6 -outer-padding 6 &

# Colors
riverctl background-color 0x2e3440
riverctl border-color-focused 0x81a1c1
riverctl border-color-unfocused 0x3b4252

# Rules
riverctl rule-add -app-id 'lxqt*' -title '*' float
riverctl rule-add -app-id 'pavucontrol' -title '*' float
INITEOF

chmod +x "$RIVER_CONFIG_DIR/init"

log_ok "Config written"
log_dim "  $RIVER_CONFIG_DIR/init"

# ---- Previews ----
echo ""
log_info "Config preview:"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$RIVER_CONFIG_DIR/init"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching River+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    unset WLR_BACKENDS
    exec "$RIVER_BIN"
else
    log_step "Launching River+LXQt nested (inside current desktop)"
    log_dim "An X11 window titled 'river' should appear."
    log_dim "Press Super+Shift+E to exit."
    log_dim ""
    export WLR_BACKENDS=x11
    exec "$RIVER_BIN"
fi
