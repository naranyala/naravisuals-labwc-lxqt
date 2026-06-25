#!/bin/bash
# Toggle redshift (night light) on/off
# Usage: toggle-redshift.sh [--on|--off|--status]

CONF_DIR="$HOME/.config/redshift"
STATE_FILE="$CONF_DIR/.state"

get_status() {
  if pgrep -x redshift >/dev/null 2>&1; then
    echo "on"
  else
    echo "off"
  fi
}

do_enable() {
  if pgrep -x redshift >/dev/null 2>&1; then
    echo "redshift is already running"
    return 0
  fi
  redshift &
  disown
  echo "redshift enabled"
}

do_disable() {
  if ! pgrep -x redshift >/dev/null 2>&1; then
    echo "redshift is not running"
    return 0
  fi
  redshift -x 2>/dev/null
  killall redshift 2>/dev/null
  echo "redshift disabled"
}

do_toggle() {
  if pgrep -x redshift >/dev/null 2>&1; then
    do_disable
  else
    do_enable
  fi
}

case "${1:-}" in
  --on)      do_enable ;;
  --off)     do_disable ;;
  --status)  get_status ;;
  --toggle)  do_toggle ;;
  *)         do_toggle ;;
esac
