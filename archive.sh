#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "$0")/scripts/installers" && pwd)"
exec bash "$SCRIPT_DIR/archive.sh" "$@"
