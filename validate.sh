#!/bin/bash
# Naravisuals Config Scanner / Validator
# Checks if LXQt + Labwc configs are present and correctly configured.
#
# Usage:
#   bash validate.sh              # Full scan
#   bash validate.sh --fix        # Auto-fix where possible
#   bash validate.sh --json       # Machine-readable output
#   bash validate.sh --quiet      # Only show failures

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/lib.sh"

FIX_MODE=false
JSON_MODE=false
QUIET_MODE=false
ERRORS=0
WARNINGS=0
PASSED=0

for arg in "$@"; do
    case "$arg" in
        --fix|-f)   FIX_MODE=true ;;
        --json|-j)  JSON_MODE=true ;;
        --quiet|-q) QUIET_MODE=true ;;
        --help|-h)
            printf "Naravisuals Config Scanner / Validator\n\n"
            printf "Usage: bash validate.sh [options]\n\n"
            printf "Options:\n"
            printf "  --fix, -f      Auto-fix issues where possible\n"
            printf "  --json, -j     Machine-readable JSON output\n"
            printf "  --quiet, -q    Only show failures\n"
            printf "  --help, -h     Show this help\n"
            exit 0
            ;;
    esac
done

# ---- Result tracking ----
results=()

pass() {
    PASSED=$((PASSED + 1))
    results+=("PASS|${1}|${2:-}")
    if [ "$QUIET_MODE" = false ] && [ "$JSON_MODE" = false ]; then
        log_ok "$1"
    fi
}

warn() {
    WARNINGS=$((WARNINGS + 1))
    results+=("WARN|${1}|${2:-}")
    if [ "$JSON_MODE" = false ]; then
        log_warn "$1"
        if [ -n "${2:-}" ]; then
            log_info "  Fix: $2"
        fi
    fi
}

fail() {
    ERRORS=$((ERRORS + 1))
    results+=("FAIL|${1}|${2:-}")
    if [ "$JSON_MODE" = false ]; then
        log_error "$1"
        if [ -n "${2:-}" ]; then
            log_info "  Fix: $2"
        fi
    fi
}

# ---- Validators ----

check_file() {
    local path="$1"
    local desc="$2"
    local fix="${3:-}"

    if [ -f "$path" ]; then
        local size
        size=$(stat -c%s "$path" 2>/dev/null || echo 0)
        if [ "$size" -gt 0 ]; then
            pass "$desc exists ($size bytes)"
        else
            fail "$desc exists but is empty" "$fix"
        fi
    else
        fail "$desc missing: $path" "$fix"
    fi
}

check_binary() {
    local name="$1"
    local desc="${2:-$1}"

    if command -v "$name" &>/dev/null; then
        pass "$desc installed: $(which $name)"
    else
        fail "$desc not found" "Install: sudo apt install $name"
    fi
}

check_env_var() {
    local file="$1"
    local var="$2"
    local expected="$3"
    local desc="$4"

    if [ ! -f "$file" ]; then
        fail "$desc — config file missing" "Deploy dotfiles first"
        return
    fi

    local actual
    actual=$(grep "^${var}=" "$file" 2>/dev/null | cut -d= -f2-)

    if [ -z "$actual" ]; then
        fail "$desc — $var not set" "Add $var=$expected to $file"
    elif [ "$actual" = "$expected" ]; then
        pass "$desc — $var=$actual"
    else
        warn "$desc — $var=$actual (expected: $expected)" "Edit $file"
    fi
}

check_autostart_entry() {
    local file="$1"
    local pattern="$2"
    local service="$3"

    if [ ! -f "$file" ]; then
        fail "Autostart file missing" "Deploy dotfiles"
        return
    fi

    if grep -q "$pattern" "$file"; then
        pass "Autostart: $service configured"
    else
        fail "Autostart: $service missing" "Add $service to $file"
    fi
}

check_panel_plugin() {
    local file="$1"
    local plugin="$2"

    if [ ! -f "$file" ]; then
        fail "Panel config missing" "Deploy dotfiles"
        return
    fi

    local plugins
    plugins=$(grep "^plugins=" "$file" | head -1 | cut -d= -f2)

    if echo "$plugins" | grep -q "$plugin"; then
        pass "Panel plugin: $plugin"
    else
        warn "Panel plugin: $plugin missing from plugins list" "Add $plugin to plugins= in $file"
    fi
}

step() {
    if [ "$JSON_MODE" = false ]; then
        log_step "$1"
    fi
}

# ==== Start scanning ====

if [ "$JSON_MODE" = false ]; then
    print_header "Naravisuals Config Scanner"
fi

# ---- 1. Required Binaries ----
if [ "$JSON_MODE" = false ]; then
    step "Checking required binaries"
fi

check_binary "labwc" "Labwc compositor"
check_binary "lxqt-session" "LXQt session"
check_binary "lxqt-panel" "LXQt panel"
check_binary "pcmanfm-qt" "File manager"
check_binary "qterminal" "Terminal emulator"
check_binary "swaybg" "Wallpaper renderer"
check_binary "swayidle" "Idle manager"
check_binary "swaylock" "Screen locker"
check_binary "grim" "Screenshot capture"
check_binary "slurp" "Area selection"
check_binary "dunst" "Notification daemon"
check_binary "wl-copy" "Clipboard copy"
check_binary "wl-paste" "Clipboard paste"
check_binary "rofi" "Application launcher"
check_binary "wob" "OSD overlay bar"
check_binary "qt6ct" "Qt6 configuration"
check_binary "pactl" "Audio control"
check_binary "brightnessctl" "Brightness control"
check_binary "playerctl" "Media control"
check_binary "nm-applet" "Network tray"
check_binary "blueman-applet" "Bluetooth tray"

# ---- 2. Required Config Files ----
if [ "$JSON_MODE" = false ]; then
    step "Checking config files"
fi

HOME_DIR="${HOME:-/home/$(whoami)}"

check_file "$HOME_DIR/.config/labwc/rc.xml" "Labwc keybindings" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/labwc/autostart" "Labwc autostart" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/labwc/environment" "Labwc environment" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/labwc/themerc-override" "Labwc window theme" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/labwc/menu.xml" "Labwc root menu" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/lxqt/panel.conf" "LXQt panel config" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/lxqt/session.conf" "LXQt session config" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/lxqt/lxqt.conf" "LXQt core config" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/lxqt/lxqt-panel.qss" "Panel QSS theme" \
    "Run: bash scripts/features/install-kvantum.sh"

check_file "$HOME_DIR/.config/gtk-3.0/settings.ini" "GTK3 settings" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/gtk-4.0/settings.ini" "GTK4 settings" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/qt6ct/qt6ct.conf" "Qt6CT config" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/dunst/dunstrc" "Dunst config" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/swaylock/config" "Swaylock config" \
    "Run: bash install.sh --minimal"

check_file "$HOME_DIR/.config/rofi/config.rasi" "Rofi config" \
    "Run: bash scripts/features/install-launcher.sh"

check_file "$HOME_DIR/.config/wob/config" "Wob config" \
    "Run: bash scripts/features/install-osd.sh"

check_file "$HOME_DIR/.config/cliphist/config" "Cliphist config" \
    "Run: bash scripts/features/install-clipboard.sh"

# ---- 3. Environment Variables ----
if [ "$JSON_MODE" = false ]; then
    step "Checking environment variables"
fi

ENV_FILE="$HOME_DIR/.config/labwc/environment"

check_env_var "$ENV_FILE" "XDG_CURRENT_DESKTOP" "LXQt:labwc:wlroots" "Desktop identifier"
check_env_var "$ENV_FILE" "XDG_SESSION_TYPE" "wayland" "Session type"
check_env_var "$ENV_FILE" "QT_QPA_PLATFORMTHEME" "qt6ct" "Qt6 platform theme"
check_env_var "$ENV_FILE" "MOZ_ENABLE_WAYLAND" "1" "Firefox Wayland"
check_env_var "$ENV_FILE" "ELECTRON_ENABLE_WAYLAND" "1" "Electron Wayland"
check_env_var "$ENV_FILE" "GDK_BACKEND" "wayland,x11" "GTK backend"
check_env_var "$ENV_FILE" "XCURSOR_THEME" "breeze_cursors" "Cursor theme"
check_env_var "$ENV_FILE" "XCURSOR_SIZE" "24" "Cursor size"
check_env_var "$ENV_FILE" "GTK_USE_PORTAL" "1" "GTK portal"
check_env_var "$ENV_FILE" "GDK_FONTCONFIG_HINT" "1" "Font rendering (GTK)"
check_env_var "$ENV_FILE" "GDK_RENDERING" "image" "Font rendering mode"
check_env_var "$ENV_FILE" "PANGOCAIRO_BACKEND" "fontconfig" "Font rendering (Pango)"

# ---- 3b. Font Rendering ----
if [ "$JSON_MODE" = false ]; then
    step "Checking font rendering"
fi

check_file "$HOME_DIR/.config/fontconfig/fonts.conf" "Fontconfig user config" \
    "Run: bash scripts/features/setup-fontconfig.sh"

# Check font availability
if command -v fc-match &>/dev/null; then
    sans_resolved=$(fc-match "sans-serif" 2>/dev/null | head -1)
    mono_resolved=$(fc-match "monospace" 2>/dev/null | head -1)
    serif_resolved=$(fc-match "serif" 2>/dev/null | head -1)
    if echo "$sans_resolved" | grep -qi "Noto Sans"; then
        pass "sans-serif resolves to Noto Sans"
    else
        warn "sans-serif resolves to: $sans_resolved" "Install: sudo apt install fonts-noto"
    fi
    if echo "$mono_resolved" | grep -qi "Noto Sans Mono\|DejaVu Sans Mono"; then
        pass "monospace resolves correctly"
    else
        warn "monospace resolves to: $mono_resolved" "Install: sudo apt install fonts-noto"
    fi
    if echo "$serif_resolved" | grep -qi "Noto Serif"; then
        pass "serif resolves to Noto Serif"
    else
        warn "serif resolves to: $serif_resolved" "Install: sudo apt install fonts-noto"
    fi
else
    fail "fc-match not found" "Install: sudo apt install fontconfig"
fi

# Check font cache is not stale (cache files newer than 7 days = OK)
if [ -d "$HOME_DIR/.cache/fontconfig" ]; then
    cache_age=$(( ($(date +%s) - $(stat -c %Y "$HOME_DIR/.cache/fontconfig" 2>/dev/null || echo 0)) / 86400 ))
    if [ "$cache_age" -lt 7 ]; then
        pass "Font cache is fresh (${cache_age} days old)"
    else
        warn "Font cache is ${cache_age} days old" "Run: fc-cache -fv"
    fi
fi

# ---- 4. Autostart Services ----
if [ "$JSON_MODE" = false ]; then
    step "Checking autostart services"
fi

AUTOSTART="$HOME_DIR/.config/labwc/autostart"

check_autostart_entry "$AUTOSTART" "swaybg" "Wallpaper (swaybg)"
check_autostart_entry "$AUTOSTART" "dunst\|lxqt-notificationd" "Notifications"
check_autostart_entry "$AUTOSTART" "lxqt-policykit-agent" "PolicyKit agent"
check_autostart_entry "$AUTOSTART" "cliphist\|copyq" "Clipboard manager"
check_autostart_entry "$AUTOSTART" "swayidle" "Screen idle"
check_autostart_entry "$AUTOSTART" "nm-applet" "Network manager"
check_autostart_entry "$AUTOSTART" "blueman-applet" "Bluetooth"
check_autostart_entry "$AUTOSTART" "xdg-desktop-portal" "Portal daemon"

# ---- 5. Panel Plugins ----
if [ "$JSON_MODE" = false ]; then
    step "Checking panel plugins"
fi

PANEL_CONF="$HOME_DIR/.config/lxqt/panel.conf"

check_panel_plugin "$PANEL_CONF" "fancymenu"
check_panel_plugin "$PANEL_CONF" "taskbar"
check_panel_plugin "$PANEL_CONF" "statusnotifier"
check_panel_plugin "$PANEL_CONF" "tray"
check_panel_plugin "$PANEL_CONF" "volume"
check_panel_plugin "$PANEL_CONF" "clock\|worldclock"
check_panel_plugin "$PANEL_CONF" "showdesktop"
check_panel_plugin "$PANEL_CONF" "desktopswitch"

# ---- 6. GTK Theme Consistency ----
if [ "$JSON_MODE" = false ]; then
    step "Checking theme consistency"
fi

GTK3="$HOME_DIR/.config/gtk-3.0/settings.ini"
GTK4="$HOME_DIR/.config/gtk-4.0/settings.ini"

if [ -f "$GTK3" ] && [ -f "$GTK4" ]; then
    gtk3_theme=$(grep "^gtk-theme-name=" "$GTK3" | cut -d= -f2)
    gtk4_theme=$(grep "^gtk-theme-name=" "$GTK4" | cut -d= -f2)
    if [ "$gtk3_theme" = "$gtk4_theme" ]; then
        pass "GTK theme consistent: $gtk3_theme"
    else
        warn "GTK theme mismatch: GTK3=$gtk3_theme, GTK4=$gtk4_theme" \
            "Edit $GTK4 to match GTK3"
    fi

    gtk3_cursor=$(grep "^gtk-cursor-theme-name=" "$GTK3" | cut -d= -f2)
    gtk4_cursor=$(grep "^gtk-cursor-theme-name=" "$GTK4" | cut -d= -f2)
    if [ "$gtk3_cursor" = "$gtk4_cursor" ]; then
        pass "GTK cursor consistent: $gtk3_cursor"
    else
        warn "GTK cursor mismatch: GTK3=$gtk3_cursor, GTK4=$gtk4_cursor" \
            "Edit $GTK4 to match GTK3"
    fi
fi

# ---- 7. System Files ----
if [ "$JSON_MODE" = false ]; then
    step "Checking system files"
fi

if [ -f "/usr/share/wayland-sessions/lxqt-labwc.desktop" ]; then
    pass "Wayland session file installed"
else
    fail "Wayland session file missing" \
        "sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/"
fi

if [ -f "/etc/sddm.conf.d/lxqt-labwc.conf" ]; then
    pass "SDDM config installed"
else
    fail "SDDM config missing" \
        "sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/"
fi

# ---- 8. Disk Space ----
if [ "$JSON_MODE" = false ]; then
    step "Checking system resources"
fi

avail=$(df -m "$HOME_DIR" 2>/dev/null | awk 'NR==2 {print $4}')
if [ -n "$avail" ] && [ "$avail" -gt 500 ]; then
    pass "Disk space: ${avail}MB available"
elif [ -n "$avail" ]; then
    warn "Low disk space: ${avail}MB available" "Free up space in $HOME_DIR"
fi

# ---- JSON Output ----
if [ "$JSON_MODE" = true ]; then
    json_escape() {
        printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
    }
    printf "{\n"
    printf "  \"passed\": %d,\n" "$PASSED"
    printf "  \"warnings\": %d,\n" "$WARNINGS"
    printf "  \"errors\": %d,\n" "$ERRORS"
    printf "  \"results\": [\n"
    for i in "${!results[@]}"; do
        IFS='|' read -r status check fix <<< "${results[$i]}"
        esc_check=$(json_escape "$check")
        esc_fix=$(json_escape "$fix")
        comma=","
        if [ "$i" -eq $((${#results[@]}-1)) ]; then
            comma=""
        fi
        printf "    {\"status\":\"%s\",\"check\":\"%s\",\"fix\":\"%s\"}%s\n" \
            "$status" "$esc_check" "$esc_fix" "$comma"
    done
    printf "  ]\n"
    printf "}\n"
    exit $ERRORS
fi

# ---- Summary ----
printf "\n"
printf "${BOLD}Validation Summary${RST}\n"
printf "%s\n" "────────────────────────────────────────"
printf "  ${GREEN}Passed:${RST}   %d\n" "$PASSED"
printf "  ${YELLOW}Warnings:${RST} %d\n" "$WARNINGS"
printf "  ${RED}Errors:${RST}   %d\n" "$ERRORS"
printf "%s\n" "────────────────────────────────────────"

if [ "$ERRORS" -eq 0 ] && [ "$WARNINGS" -eq 0 ]; then
    printf "\n${BOLD}${GREEN}All checks passed. Configuration is healthy.${RST}\n"
elif [ "$ERRORS" -eq 0 ]; then
    printf "\n${BOLD}${YELLOW}No errors, but %d warning(s) found.${RST}\n" "$WARNINGS"
    printf "Run with --fix to auto-resolve where possible.\n"
else
    printf "\n${BOLD}${RED}%d error(s) found. Configuration needs attention.${RST}\n" "$ERRORS"
    printf "Run with --fix to auto-resolve where possible.\n"
    printf "Or re-run: bash install.sh --full\n"
fi

exit $ERRORS
