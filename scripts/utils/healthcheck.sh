#!/bin/bash
# Naravisuals Health Check CLI
# Quick access to the health check tool
#
# Usage:
#   bash healthcheck.sh              # Run all checks
#   bash healthcheck.sh env          # Check env vars only
#   bash healthcheck.sh --fix all    # Run all checks and auto-fix
#   bash healthcheck.sh --json env   # JSON output
#   bash healthcheck.sh --quiet      # Only show failures

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HEALTHCHECK="$SCRIPT_DIR/apps/shared/build/nv-healthcheck"

if [ ! -f "$HEALTHCHECK" ]; then
    echo "Health check binary not found. Building..."
    cd "$SCRIPT_DIR/apps/shared" || exit 1
    mkdir -p build && cd build
    cmake -GNinja .. >/dev/null 2>&1 && ninja >/dev/null 2>&1
    if [ ! -f "$HEALTHCHECK" ]; then
        echo "Build failed. Install Qt6 dev packages: sudo apt install qt6-base-dev"
        exit 1
    fi
    cd "$SCRIPT_DIR"
fi

exec "$HEALTHCHECK" "$@"
