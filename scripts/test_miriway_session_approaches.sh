#!/bin/bash
# Compare Miriway session management approaches for LXQt.
# Tests two approaches side by side:
#   Approach A: Individual shell-components (panel, notificationd, policykit)
#   Approach B: Full lxqt-session as the sole shell-component
#
# Usage:
#   bash test_miriway_session_approaches.sh [--a|--b|--both]
#
# Each approach runs for 15 seconds. Watch for:
#   - Does the panel appear?
#   - Does notificationd start?
#   - Does lxqt-session launch autostart apps?
#   - Any errors in the output?
# Press Ctrl+C to abort early.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

MIRIWAY_SHELL="${MIRIWAY_SHELL:-$(command -v miriway-shell)}"
TIMEOUT="${TIMEOUT:-15}"
USE_DISPLAY="${DISPLAY:-:0}"

require_cmd() {
    if ! command -v "$1" &>/dev/null; then
        log_error "$1 not found"
        exit 1
    fi
}

require_cmd miriway-shell
require_cmd swaybg

run_approach() {
    local label="$1" config_file="$2" extra_env="${3:-}"

    local tmpdir
    tmpdir="$(mktemp -d)"
    local rt_dir="$tmpdir/runtime"
    mkdir -p "$rt_dir" "$tmpdir/config"

    # Write the config
    cp "$config_file" "$tmpdir/config/miriway-shell.config"

    # Also create empty settings to prevent migration from blocking
    touch "$tmpdir/config/miriway-shell.settings"

    echo ""
    log_step "Approach $label"
    log_info "Config: $config_file"
    log_dim "$(sed 's/^/  │ /' "$config_file")"
    echo ""

    env_vars=(
        "XDG_CONFIG_HOME=$tmpdir/config"
        "XDG_RUNTIME_DIR=$rt_dir"
        "DISPLAY=$USE_DISPLAY"
        "MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:x11"
        "MIR_SERVER_ENABLE_X11=1"
        "XDG_CURRENT_DESKTOP=LXQt:Miriway:wlroots"
    )
    if [ -n "$extra_env" ]; then
        IFS=':' read -ra extra <<< "$extra_env"
        for e in "${extra[@]}"; do
            env_vars+=("$e")
        done
    fi

    # Remove WAYLAND_DISPLAY to force X11 nested mode
    export XDG_CONFIG_HOME="$tmpdir/config"
    export XDG_RUNTIME_DIR="$rt_dir"
    export DISPLAY="$USE_DISPLAY"
    export MIR_SERVER_PLATFORM_DISPLAY_LIBS="mir:x11"
    export MIR_SERVER_ENABLE_X11="1"
    export XDG_CURRENT_DESKTOP="LXQt:Miriway:wlroots"
    unset WAYLAND_DISPLAY

    log_info "Starting Miriway (nested, X11 backend)..."
    log_dim "Will stop after ${TIMEOUT}s or press Ctrl+C"

    # Run with timeout
    set +e
    timeout "$TIMEOUT" "$MIRIWAY_SHELL" 2>&1 | while IFS= read -r line; do
        case "$line" in
            *"ERROR"*|*"FATAL"*) log_error "$line" ;;
            *"WARNING"*)         log_warn "$line" ;;
            *)                   log_dim "  $line" ;;
        esac
    done
    local exit_code=$?
    set -e

    if [ "$exit_code" -eq 124 ]; then
        log_ok "Miriway ran for ${TIMEOUT}s (expected timeout)"
    elif [ "$exit_code" -eq 0 ]; then
        log_warn "Miriway exited cleanly (may have stopped early)"
    else
        log_warn "Miriway exited with code $exit_code"
    fi

    # Check if LXQt processes were started
    echo ""
    log_info "Process check (inside Miriway session):"
    for proc in lxqt-panel lxqt-notificationd lxqt-policykit-agent lxqt-session swaybg; do
        if pgrep -x "$proc" &>/dev/null; then
            log_ok "  $proc is running"
        else
            log_dim "  $proc is NOT running"
        fi
    done

    rm -rf "$tmpdir"
}

# ---- Config files ----
CONFIG_A=$(mktemp)
cat > "$CONFIG_A" << 'CONFIGEOF'
x11-window-title=Test Miriway+LXQt (shell-components)
idle-timeout=0
env-hacks=MIR_ANCHOR_RECTANGLE_UNCONSTRAINED=1
app-env-amend=XDG_SESSION_TYPE=wayland:GTK_USE_PORTAL=0:XDG_CURRENT_DESKTOP=LXQt:Miriway:wlroots:GTK_A11Y=none

shell-component=swaybg --mode fill --output '*' --color 2e3440
shell-component=miriway-unsnap lxqt-notificationd
shell-component=miriway-unsnap lxqt-policykit-agent
shell-component=miriway-unsnap lxqt-panel

ctrl-alt=BackSpace:@exit
CONFIGEOF

CONFIG_B=$(mktemp)
cat > "$CONFIG_B" << 'CONFIGEOF'
x11-window-title=Test Miriway+LXQt (full session)
idle-timeout=0
env-hacks=MIR_ANCHOR_RECTANGLE_UNCONSTRAINED=1
app-env-amend=XDG_SESSION_TYPE=wayland:GTK_USE_PORTAL=0:XDG_CURRENT_DESKTOP=LXQt:Miriway:wlroots:GTK_A11Y=none

shell-component=miriway-unsnap lxqt-session

ctrl-alt=BackSpace:@exit
CONFIGEOF

# ---- Run ----
MODE="${1:---both}"

print_header "Miriway Session Approach Comparison"

echo ""
echo "Approach A: Individual shell-components"
echo "  Runs swaybg, lxqt-notificationd, lxqt-policykit-agent, lxqt-panel"
echo "  directly as shell-components in miriway-shell.config."
echo "  Pros: Each component runs independently, no session-manager overhead."
echo "  Cons: No lxqt-session autostart, no session settings, no leave dialog."
echo ""
echo "Approach B: Full lxqt-session"
echo "  Runs lxqt-session as the sole shell-component; lxqt-session handles"
echo "  launching panel, notificationd, etc. via its own autostart mechanism."
echo "  Pros: Full LXQt session management, autostart, settings GUI."
echo "  Cons: lxqt-session may try to manage a WM; more moving parts."
echo ""

case "$MODE" in
    --a|--A|-a)
        run_approach "A" "$CONFIG_A"
        ;;
    --b|--B|-b)
        run_approach "B" "$CONFIG_B"
        ;;
    --both|*)
        run_approach "A" "$CONFIG_A"
        sleep 2
        run_approach "B" "$CONFIG_B"
        ;;
esac

rm -f "$CONFIG_A" "$CONFIG_B"

echo ""
print_summary "Comparison Complete" "info"
echo ""
echo "Summary of differences:"
echo "  Approach A | Approach B"
echo "  -----------+-----------"
echo "  Direct     | Through lxqt-session"
echo "  No session | Full session management"
echo "  Fast start | Slower start (session init)"
echo "  Manual     | Autostart support"
echo "  No WM mgmt | lxqt-session WM detection"
echo ""
echo "See scripts/launch_miriway_lxqt_test.sh for the full LXQt test session."
