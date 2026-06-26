#!/bin/bash
# Toggle LXQt Panel on/off
# Kills the panel if running, or starts it if not.
#
# Usage:
#   bash toggle-lxqt-panel.sh          # Toggle panel
#   bash toggle-lxqt-panel.sh --start  # Start only (no-op if running)
#   bash toggle-lxqt-panel.sh --stop   # Stop only (no-op if not running)
#   bash toggle-lxqt-panel.sh --status # Check if running

set -euo pipefail

PANEL_BIN="lxqt-panel"
ACTION="toggle"

for arg in "$@"; do
    case "$arg" in
        --start)  ACTION="start" ;;
        --stop)   ACTION="stop" ;;
        --status) ACTION="status" ;;
        --help|-h)
            printf "Toggle LXQt Panel\n\n"
            printf "Usage: bash %s [options]\n\n" "$(basename "$0")"
            printf "Options:\n"
            printf "  (none)     Toggle panel on/off\n"
            printf "  --start    Start panel only\n"
            printf "  --stop     Stop panel only\n"
            printf "  --status   Show if panel is running\n"
            printf "  --help     Show this help\n"
            exit 0
            ;;
    esac
done

is_running() {
    pidof "$PANEL_BIN" &>/dev/null
}

case "$ACTION" in
    status)
        if is_running; then
            printf "lxqt-panel is running (PID %s)\n" "$(pidof "$PANEL_BIN")"
            exit 0
        else
            printf "lxqt-panel is not running\n"
            exit 1
        fi
        ;;
    stop)
        if is_running; then
            killall -9 "$PANEL_BIN" 2>/dev/null || true
            for i in $(seq 1 20); do
                is_running || break
                sleep 0.1
            done
            printf "lxqt-panel stopped\n"
        else
            printf "lxqt-panel is not running\n"
        fi
        ;;
    start)
        if is_running; then
            printf "lxqt-panel is already running (PID %s)\n" "$(pidof "$PANEL_BIN")"
        else
            "$PANEL_BIN" &>/dev/null &
            disown
            sleep 0.5
            if is_running; then
                printf "lxqt-panel started (PID %s)\n" "$(pidof "$PANEL_BIN")"
            else
                printf "Failed to start lxqt-panel\n"
                exit 1
            fi
        fi
        ;;
    toggle)
        if is_running; then
            killall -9 "$PANEL_BIN" 2>/dev/null || true
            for i in $(seq 1 20); do
                is_running || break
                sleep 0.1
            done
            printf "lxqt-panel stopped\n"
        else
            "$PANEL_BIN" &>/dev/null &
            disown
            sleep 0.5
            if is_running; then
                printf "lxqt-panel started (PID %s)\n" "$(pidof "$PANEL_BIN")"
            else
                printf "Failed to start lxqt-panel\n"
                exit 1
            fi
        fi
        ;;
esac
