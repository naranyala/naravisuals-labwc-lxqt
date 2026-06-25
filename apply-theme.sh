#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")/scripts/themes" && pwd)"
exec bash "$SCRIPT_DIR/apply-theme.sh" "$@"
