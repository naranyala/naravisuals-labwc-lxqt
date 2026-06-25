#!/bin/bash
# Naravisuals — Night Light Toggle (wlsunset)
# =============================================
# Toggles wlsunset on/off with a smooth temperature transition.
# Supports latitude/longitude for auto sunset/sunrise timing.
#
# Usage:
#   bash night-light.sh              # Toggle
#   bash night-light.sh on           # Enable
#   bash night-light.sh off          # Disable
#   bash night-light.sh status       # Check if running
#   bash night-light.sh temp 3500    # Set temperature (1000-10000)
#
# Default: 3500K night, latitude=51.5 (London), longitude=-0.12
# Adjust lat/long below for your location.

LAT="51.5"
LON="-0.12"
NIGHT_TEMP=3500
DAY_TEMP=6500
PIDFILE="${XDG_RUNTIME_DIR:-/tmp}/wlsunset.pid"

start_wlsunset() {
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
        echo "wlsunset already running"
        exit 0
    fi
    wlsunset -l "$LAT" -L "$LON" -t "$NIGHT_TEMP" -T "$DAY_TEMP" &
    echo $! > "$PIDFILE"
    echo "wlsunset started (night: ${NIGHT_TEMP}K, day: ${DAY_TEMP}K)"
}

stop_wlsunset() {
    if [ -f "$PIDFILE" ]; then
        pid=$(cat "$PIDFILE")
        kill "$pid" 2>/dev/null && echo "wlsunset stopped"
        rm -f "$PIDFILE"
    else
        pkill wlsunset 2>/dev/null && echo "wlsunset stopped" || echo "wlsunset not running"
    fi
}

case "${1:-toggle}" in
    on)  start_wlsunset ;;
    off) stop_wlsunset ;;
    status)
        if pgrep wlsunset &>/dev/null; then
            echo "active"
        else
            echo "inactive"
        fi
        ;;
    temp)
        if [ -n "$2" ]; then
            NIGHT_TEMP=$2
            stop_wlsunset
            start_wlsunset
        fi
        ;;
    toggle|*)
        if pgrep wlsunset &>/dev/null; then
            stop_wlsunset
        else
            start_wlsunset
        fi
        ;;
esac
