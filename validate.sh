#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")/scripts/utils" && pwd)"
exec bash "$SCRIPT_DIR/validate.sh" "$@"
