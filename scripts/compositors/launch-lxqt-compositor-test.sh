#!/bin/bash
# Launch any Wayland compositor with LXQt desktop components for testing.
# Runs nested inside the current X11 session so no VT switch is needed.
#
# Usage:
#   bash launch-lxqt-compositor-test.sh <compositor> [--yes]
#   bash launch-lxqt-compositor-test.sh --list
#
# Supported compositors:
#   miriway    Mir-based floating (proven)
#   labwc      Openbox-for-Wayland stacking
#   sway       i3-compatible tiling
#   wayfire    3D plugin-based stacking
#   hyprland   Animated dynamic tiling
#   niri       Scrollable tiling
#   river      Dynamic tiling

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

COMPOSITOR=""
INTERACTIVE=true
COMPOSITOR_ARGS=""

# ---- Detect which compositor is being requested ----
for arg in "$@"; do
    case "$arg" in
        --yes|-y)      INTERACTIVE=false ;;
        --help|-h)
            printf "Usage: bash launch-lxqt-compositor-test.sh <compositor> [--yes]\n"
            printf "  bash launch-lxqt-compositor-test.sh --list\n"
            exit 0 ;;
        --list)
            printf "Available compositors for nested testing:\n"
            printf "  %-12s %s\n" "miriway"  "Mir-based floating  (env: MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:x11)"
            printf "  %-12s %s\n" "labwc"    "wlroots stacking   (env: WLR_BACKENDS=x11)"
            printf "  %-12s %s\n" "sway"     "wlroots tiling     (env: WLR_BACKENDS=x11)"
            printf "  %-12s %s\n" "wayfire"  "wlroots 3D stacking(env: WLR_BACKENDS=x11)"
            printf "  %-12s %s\n" "hyprland" "wlroots tiling     (env: WLR_BACKENDS=x11)"
            printf "  %-12s %s\n" "niri"     "wlroots tiling     (flag: --x11)"
            printf "  %-12s %s\n" "river"    "wlroots tiling     (env: WLR_BACKENDS=x11)"
            printf "\nAll run nested in an X11 window. Press Ctrl+Alt+BackSpace or Super+Shift+E to exit.\n"
            exit 0 ;;
        miriway|labwc|sway|wayfire|hyprland|niri|river)
            COMPOSITOR="$arg" ;;
        *)
            COMPOSITOR_ARGS="$COMPOSITOR_ARGS $arg" ;;
    esac
done

if [ -z "$COMPOSITOR" ]; then
    log_error "Specify a compositor: miriway, labwc, sway, wayfire, hyprland, niri, or river"
    log_info "Run with --list to see options"
    exit 1
fi

# ---- Map compositor details ----
declare -A BIN_NAMES=(
    [miriway]="miriway"
    [labwc]="labwc"
    [sway]="sway"
    [wayfire]="wayfire"
    [hyprland]="Hyprland"
    [niri]="niri"
    [river]="river"
)

declare -A CONFIG_PATHS=(
    [miriway]="$HOME/.config/miriway-shell.config"
    [labwc]="$HOME/.config/labwc/rc.xml"
    [sway]="$HOME/.config/sway/config"
    [wayfire]="$HOME/.config/wayfire.ini"
    [hyprland]="$HOME/.config/hypr/hyprland.conf"
    [niri]="$HOME/.config/niri/config.kdl"
    [river]="$HOME/.config/river/init"
)

declare -A CONFIG_TEMPLATES=(
    [miriway]="configs/compositors/miriway/miriway-shell.config"
    [labwc]=""
    [sway]="configs/compositors/sway/config"
    [wayfire]="configs/compositors/wayfire/wayfire.ini"
    [hyprland]="configs/compositors/hyprland/hyprland.conf"
    [niri]="configs/compositors/niri/lxqt-niri.kdl"
    [river]="configs/compositors/river/lxqt-river-init"
)

BIN="${BIN_NAMES[$COMPOSITOR]}"
BIN_PATH="$(command -v "$BIN" 2>/dev/null || true)"

# ---- Check prerequisites ----
if [ -z "$BIN_PATH" ]; then
    log_error "$BIN not found in PATH"
    exit 1
fi

log_info "${COMPOSITOR}: $BIN_PATH"

LXQT_DEPS=(swaybg)
MISSING=()
for dep in "${LXQT_DEPS[@]}"; do
    command -v "$dep" &>/dev/null || MISSING+=("$dep")
done
if [[ ${#MISSING[@]} -gt 0 ]]; then
    log_warn "Missing deps: ${MISSING[*]}"
    if $INTERACTIVE; then
        read -rp "Continue anyway? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

# ---- Write a temporary test config if needed ----
TMPDIR=""
CONFIG_WARN=false

if [ "$COMPOSITOR" = "miriway" ]; then
    # Miriway uses its own config file format with shell-component=
    CFG="$HOME/.config/miriway-shell.config"
    SET="$HOME/.config/miriway-shell.settings"
    if [[ -f "$CFG" || -f "$SET" ]]; then
        CONFIG_WARN=true
    fi
    if $CONFIG_WARN && $INTERACTIVE; then
        read -rp "Overwrite existing miriway config? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
    cat > "$CFG" << CONFIGEOF
x11-window-title=LXQt on ${COMPOSITOR}
idle-timeout=600
env-hacks=MIR_ANCHOR_RECTANGLE_UNCONSTRAINED=1
app-env-amend=XDG_SESSION_TYPE=wayland:GTK_USE_PORTAL=0:XDG_CURRENT_DESKTOP=LXQt:${COMPOSITOR}:wlroots:GTK_A11Y=none

shell-component=swaybg --mode fill --output '*' --color 2e3440
shell-component=miriway-unsnap lxqt-notificationd
shell-component=miriway-unsnap lxqt-policykit-agent
shell-component=miriway-unsnap lxqt-panel
CONFIGEOF
    cat > "$SET" << 'SETTINGSEOF'
command_ctrl_alt=t:miriway-unsnap qterminal
command_shell_meta=a:miriway-unsnap lxqt-runner
command_shell_plain=F12:miriway-unsnap qterminal -d
command_meta=Left:@dock-left
command_meta=Right:@dock-right
command_meta=Space:@toggle-maximized
command_ctrl_alt=BackSpace:@exit
SETTINGSEOF
    log_ok "Wrote test config for miriway"
else
    # For wlroots compositors, preferably use existing config or warn
    CONFIG_PATH="${CONFIG_PATHS[$COMPOSITOR]}"
    if [ ! -f "$CONFIG_PATH" ]; then
        TEMPLATE="${CONFIG_TEMPLATES[$COMPOSITOR]}"
        if [ -n "$TEMPLATE" ] && [ -f "$SCRIPT_DIR/../$TEMPLATE" ]; then
            mkdir -p "$(dirname "$CONFIG_PATH")"
            cp "$SCRIPT_DIR/../$TEMPLATE" "$CONFIG_PATH"
            log_ok "Copied default config from $TEMPLATE to $CONFIG_PATH"
        else
            CONFIG_WARN=true
        fi
    fi
    if $CONFIG_WARN && $INTERACTIVE; then
        log_warn "No config found for $COMPOSITOR at $CONFIG_PATH"
        log_warn "The compositor may use defaults or fail to start."
        read -rp "Continue anyway? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] || exit 1
    fi
fi

# ---- Launch nested ----
echo ""
log_step "Launching $COMPOSITOR + LXQt nested (X11 window)"
log_dim "Window title: 'LXQt on $COMPOSITOR'"
log_dim "Press Ctrl+Alt+BackSpace to exit (check compositor docs for alternatives)"
echo ""

# Build env for nested mode
export XDG_CURRENT_DESKTOP="LXQt:$COMPOSITOR:wlroots"
export DISPLAY="${DISPLAY:-:0}"

case "$COMPOSITOR" in
    miriway)
        unset WAYLAND_DISPLAY
        export MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:x11
        export MIR_SERVER_ENABLE_X11=1
        exec "$BIN_PATH" $COMPOSITOR_ARGS
        ;;
    niri)
        # Niri has a native --x11 flag for nested mode
        unset WAYLAND_DISPLAY
        exec "$BIN_PATH" --x11 $COMPOSITOR_ARGS
        ;;
    *)
        # Generic wlroots: WLR_BACKENDS=x11 runs as an X11 client window
        unset WAYLAND_DISPLAY
        export WLR_BACKENDS=x11
        exec "$BIN_PATH" $COMPOSITOR_ARGS
        ;;
esac
