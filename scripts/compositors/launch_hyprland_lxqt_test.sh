#!/bin/bash
# Launch Hyprland compositor with LXQt desktop components for testing.
# Can run nested (inside an existing X11/Wayland session) or standalone.
#
# Usage:
#   ./launch_hyprland_lxqt_test.sh            # Interactive, ask before overwriting
#   ./launch_hyprland_lxqt_test.sh --yes      # Non-interactive, overwrite config
#   ./launch_hyprland_lxqt_test.sh --cleanup  # Remove generated config files
#   ./launch_hyprland_lxqt_test.sh --standalone  # Launch on bare VT (needs root/tty)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

HYPRLAND_BIN="${HYPRLAND_BIN:-$(command -v Hyprland)}"

XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
HYPRLAND_CONFIG_DIR="$XDG_CONFIG_HOME/hypr"

INTERACTIVE=true
STANDALONE=false

cleanup() {
    log_step "Cleaning up Hyprland LXQt test config"
    rm -rf "$HYPRLAND_CONFIG_DIR"
    log_ok "Removed $HYPRLAND_CONFIG_DIR"
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
if ! command -v "$HYPRLAND_BIN" &>/dev/null; then
    log_error "Hyprland not found. Install it first:"
    log_error "  sudo apt install hyprland"
    exit 1
fi

log_info "Hyprland: $(basename "$HYPRLAND_BIN")"

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
if [[ -d "$HYPRLAND_CONFIG_DIR" ]]; then
    log_warn "Existing Hyprland config directory found"
    if $INTERACTIVE; then
        read -rp "Overwrite? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

log_step "Writing Hyprland config for LXQt"

mkdir -p "$HYPRLAND_CONFIG_DIR"

cat > "$HYPRLAND_CONFIG_DIR/hyprland.conf" << 'CONFIGEOF'
# Hyprland configuration for LXQt testing

monitor = , preferred, auto, 1

exec-once = lxqt-notificationd
exec-once = lxqt-policykit-agent
exec-once = lxqt-panel
exec-once = swaybg -o '*' -c 2e3440

env = XDG_CURRENT_DESKTOP,LXQt:Hyprland
env = XDG_SESSION_DESKTOP,lxqt-hyprland
env = XDG_SESSION_TYPE,wayland

input {
    kb_layout = us
    follow_mouse = 1
    touchpad {
        natural_scroll = no
        tap-to-click = true
        disable_while_typing = true
        scroll_method = two_finger
        clickfinger_behavior = true
    }
}

general {
    gaps_in = 8
    gaps_out = 8
    border_size = 2
    col.active_border = rgba(81a1c1ff)
    col.inactive_border = rgba(2e3440ff)
    layout = master
    allow_tearing = false
}

decoration {
    rounding = 10
    shadow {
        enabled = true
        range = 20
        render_power = 3
        color = rgba(1a1a1a99)
    }
}

animations {
    enabled = true
}

$mainMod = SUPER

bind = $mainMod, Return, exec, qterminal
bind = $mainMod SHIFT, Return, exec, qterminal --drop-down
bind = $mainMod, D, exec, lxqt-runner
bind = $mainMod, P, exec, pcmanfm-qt
bind = $mainMod, Q, killactive
bind = $mainMod, M, togglefloating
bind = $mainMod, F, fullscreen
bind = $mainMod, L, exec, lxqt-leave
bind = $mainMod, S, exec, hyprctl dispatch exit

bind = $mainMod, left, movefocus, l
bind = $mainMod, right, movefocus, r
bind = $mainMod, up, movefocus, u
bind = $mainMod, down, movefocus, d

bind = $mainMod, 1, workspace, 1
bind = $mainMod, 2, workspace, 2
bind = $mainMod, 3, workspace, 3
bind = $mainMod, 4, workspace, 4

bind = $mainMod SHIFT, 1, movetoworkspace, 1
bind = $mainMod SHIFT, 2, movetoworkspace, 2
bind = $mainMod SHIFT, 3, movetoworkspace, 3
bind = $mainMod SHIFT, 4, movetoworkspace, 4

bind = , Print, exec, grimshot copy screen
bind = SHIFT, Print, exec, grimshot copy area

binde = XF86AudioRaiseVolume, exec, pactl set-sink-volume @DEFAULT_SINK@ +5%
binde = XF86AudioLowerVolume, exec, pactl set-sink-volume @DEFAULT_SINK@ -5%
bind = XF86AudioMute, exec, pactl set-sink-mute @DEFAULT_SINK@ toggle

bind = $mainMod, mouse:272, movewindow
bind = $mainMod, mouse:273, resizewindow
CONFIGEOF

log_ok "Config written"
log_dim "  $HYPRLAND_CONFIG_DIR/hyprland.conf"

# ---- Previews ----
echo ""
log_info "Config preview:"
echo "────────────────────────────────────────"
sed 's/^/  │ /' "$HYPRLAND_CONFIG_DIR/hyprland.conf"
echo "────────────────────────────────────────"

# ---- Launch ----
if $STANDALONE; then
    log_step "Launching Hyprland+LXQt standalone session"
    log_dim "(Switch to a free VT first, or run from your DM)"
    unset WLR_BACKENDS
    exec "$HYPRLAND_BIN"
else
    log_step "Launching Hyprland+LXQt nested (inside current desktop)"
    log_dim "An X11 window titled 'Hyprland' should appear."
    log_dim "Press Super+M to exit."
    log_dim ""
    export WLR_BACKENDS=x11
    exec "$HYPRLAND_BIN"
fi
