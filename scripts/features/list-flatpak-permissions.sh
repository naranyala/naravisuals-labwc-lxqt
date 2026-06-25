#!/bin/bash
# Naravisuals — Flatpak Portal Permissions Viewer
# ==================================================
# Lists installed flatpaks and their portal permissions.
#
# Usage:
#   bash list-flatpak-permissions.sh              # Full report
#   bash list-flatpak-permissions.sh --summary    # App names only
#   bash list-flatpak-permissions.sh --app <id>   # Single app
#   bash list-flatpak-permissions.sh --help       # This help

set -euo pipefail

if ! command -v flatpak &>/dev/null; then
    echo "flatpak not installed — no Flatpak applications to inspect."
    exit 0
fi

case "${1:-}" in
    --help|-h)
        echo "Flatpak Portal Permissions Viewer"
        echo ""
        echo "Usage: bash list-flatpak-permissions.sh [options]"
        echo ""
        echo "Options:"
        echo "  --summary      List installed apps with IDs only"
        echo "  --app <id>     Show permissions for a specific app"
        echo "  --help, -h     Show this help"
        exit 0
        ;;
    --summary)
        flatpak list --columns=application,name --no-login-handler 2>/dev/null || \
            flatpak list --columns=application,name 2>/dev/null
        exit 0
        ;;
    --app)
        if [ -z "${2:-}" ]; then
            echo "Usage: $0 --app <application-id>"
            exit 1
        fi
        flatpak info -m "$2" 2>/dev/null
        exit 0
        ;;
    *)
        echo "============================================"
        echo " Installed Flatpaks & Portal Permissions"
        echo "============================================"
        echo ""

        INSTALLED=$(flatpak list --columns=application --no-login-handler 2>/dev/null || flatpak list --columns=application 2>/dev/null)
        if [ -z "$INSTALLED" ]; then
            echo "No Flatpak applications installed."
            exit 0
        fi

        echo "$INSTALLED" | while read -r app; do
            [ -z "$app" ] && continue
            echo "────────────────────────────────────────"
            echo "  App: $app"
            echo "────────────────────────────────────────"
            # Show permissions from the installed ref's metadata
            flatpak info -m "$app" 2>/dev/null | grep -E "^    (filesystems|session-bus|system-bus|devices|features|env|socket|share)" | head -20 || echo "    (no portal permissions metadata)"
            echo ""
        done
        echo "============================================"
        echo "Run with --app <id> for full metadata."
        echo "============================================"
        ;;
esac
