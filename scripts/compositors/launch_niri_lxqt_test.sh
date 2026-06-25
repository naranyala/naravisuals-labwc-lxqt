#!/bin/bash
# Launch Niri compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_niri_lxqt_test.sh             # Interactive, ask before overwriting
#   ./launch_niri_lxqt_test.sh --yes       # Non-interactive, overwrite config
#   ./launch_niri_lxqt_test.sh --cleanup   # Remove generated config files
#   ./launch_niri_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

NIRI_BIN="${NIRI_BIN:-$(command -v niri)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
NIRI_CONFIG_DIR="$XDG_CONFIG_HOME/niri"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up Niri LXQt test config"
    rm -rf "$NIRI_CONFIG_DIR"
    log_ok "Removed $NIRI_CONFIG_DIR"
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
if ! command -v "$NIRI_BIN" &>/dev/null; then
    log_error "Niri not found. Install it first:"
    log_error "  sudo apt install niri"
    exit 1
fi

NIRI_VER=$("$NIRI_BIN" --version 2>/dev/null || echo "unknown")
log_info "Niri: $NIRI_VER"

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
if [[ -d "$NIRI_CONFIG_DIR" ]]; then
    log_warn "Existing Niri config directory found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

log_step "Writing Niri config for LXQt"

mkdir -p "$NIRI_CONFIG_DIR"

cat > "$NIRI_CONFIG_DIR/config.kdl" << 'CONFIGEOF'
// Niri configuration for LXQt testing

spawn-at-startup "lxqt-notificationd"
spawn-at-startup "lxqt-policykit-agent"
spawn-at-startup "lxqt-panel"
spawn-at-startup "swaybg -o '*' -c 2e3440"

layout {
    gaps 16
    focus-ring { width 2; active-color "#81a1c1"; inactive-color "#2e3440"; }
    shadow { on; softness 40; spread 5; color "#00000064"; }
    center-focused-column "on-overflow"
    default-column-width { proportion 0.5; }
}

input {
    keyboard { xkb { layout "us"; options "grp:alt_shift_toggle,compose:ralt"; } }
    touchpad { tap; natural-scroll; accel-speed 0.6; }
}

binds {
    Mod+T { spawn-sh "lxqt-qdbus run qterminal"; }
    F12 { spawn-sh "qterminal -d"; }
    Mod+D { spawn "lxqt-runner"; }
    Mod+P { spawn "pcmanfm-qt"; }
    Mod+L { spawn "lxqt-leave"; }
    Mod+Shift+E { quit; }
    Ctrl+Alt+Delete { quit; }
    Ctrl+Alt+BackSpace { quit; }

    XF86AudioRaiseVolume { spawn-sh "pactl set-sink-volume @DEFAULT_SINK@ +5%"; }
    XF86AudioLowerVolume { spawn-sh "pactl set-sink-volume @DEFAULT_SINK@ -5%"; }
    XF86AudioMute { spawn-sh "pactl set-sink-mute @DEFAULT_SINK@ toggle"; }

    Print { screenshot; }
    Ctrl+Print { screenshot-screen; }
}

window-rule {
    match app-id=r#"^lxqt-.*"#
    open-floating true
}

layer-rule {
    match namespace="^wallpaper$"
    place-within-backdrop true
}
CONFIGEOF

log_ok "Config written"
log_dim "  $NIRI_CONFIG_DIR/config.kdl"

# ---- Previews ----
echo ""
log_info "Config preview:"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$NIRI_CONFIG_DIR/config.kdl"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching Niri+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    exec "$NIRI_BIN"
else
    log_step "Launching Niri+LXQt nested (inside current desktop)"
    log_dim "An X11 window should appear."
    log_dim "Press Ctrl+Alt+Delete to exit."
    log_dim ""
    exec "$NIRI_BIN" --x11
fi
