#!/bin/bash
# Setup XDG Desktop Portal for screen sharing and file dialogs
# Required for Flatpak, screen capture, and portal-based file choosers

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

print_header "XDG Desktop Portal Setup"

# --- Detect and install ---
PM="$(detect_pm)"

install_packages() {
    case "$PM" in
        apt)
            sudo apt-get install -y xdg-desktop-portal xdg-desktop-portal-wlr xdg-desktop-portal-gtk 2>/dev/null
            ;;
        dnf)
            sudo dnf install -y xdg-desktop-portal xdg-desktop-portal-wlr xdg-desktop-portal-gtk 2>/dev/null
            ;;
        pacman)
            sudo pacman -S --noconfirm xdg-desktop-portal xdg-desktop-portal-wlr xdg-desktop-portal-gtk 2>/dev/null
            ;;
        *)
            log_warn "Install manually:"
            log_info "  xdg-desktop-portal: https://github.com/flatpak/xdg-desktop-portal"
            log_info "  xdg-desktop-portal-wlr: https://github.com/emersion/xdg-desktop-portal-wlr"
            return 1
            ;;
    esac
}

install_packages

# --- Verify ---
if [ -f /usr/libexec/xdg-desktop-portal ] || [ -f /usr/lib/xdg-desktop-portal ] || command -v xdg-desktop-portal &>/dev/null; then
    log_ok "xdg-desktop-portal installed"
else
    log_warn "xdg-desktop-portal not found — install manually"
fi

if [ -f /usr/libexec/xdg-desktop-portal-wlr ] || [ -f /usr/lib/xdg-desktop-portal-wlr ] || command -v xdg-desktop-portal-wlr &>/dev/null; then
    log_ok "xdg-desktop-portal-wlr installed (screen sharing for wlroots)"
else
    log_warn "xdg-desktop-portal-wlr not found — screen sharing may not work"
fi

# --- Setup wlr portal config ---
WLR_DIR="$HOME/.config/xdg-desktop-portal-wlr"
mkdir -p "$WLR_DIR"

PROJECT_DIR="${SCRIPT_DIR}/../.."
if [ -f "$PROJECT_DIR/configs/dotfiles/xdg-desktop-portal-wlr/config" ]; then
    cp "$PROJECT_DIR/configs/dotfiles/xdg-desktop-portal-wlr/config" "$WLR_DIR/config"
    log_ok "Installed wlr portal config"
fi

# --- Ensure environment variables are set ---
ENV_FILE="$HOME/.config/labwc/environment"
if [ -f "$ENV_FILE" ]; then
    # Add XDG_CURRENT_DESKTOP if not set
    if ! grep -q "XDG_CURRENT_DESKTOP" "$ENV_FILE"; then
        echo "XDG_CURRENT_DESKTOP=LXQt:labwc:wlroots" >> "$ENV_FILE"
        log_ok "Added XDG_CURRENT_DESKTOP"
    fi

    # Ensure GTK_USE_PORTAL is set
    if ! grep -q "GTK_USE_PORTAL" "$ENV_FILE"; then
        echo "GTK_USE_PORTAL=1" >> "$ENV_FILE"
        log_ok "Added GTK_USE_PORTAL=1"
    fi
fi

# --- Add portal startup to labwc autostart if missing ---
AUTOSTART="$HOME/.config/labwc/autostart"
if [ -f "$AUTOSTART" ]; then
    if ! grep -q "xdg-desktop-portal" "$AUTOSTART"; then
        log_info "Adding portal startup to labwc autostart..."
        cat >> "$AUTOSTART" << 'AUTO'

# XDG Desktop Portals (file chooser, screen sharing)
if command -v xdg-desktop-portal &>/dev/null; then
  /usr/libexec/xdg-desktop-portal &
fi
if command -v xdg-desktop-portal-wlr &>/dev/null; then
  /usr/libexec/xdg-desktop-portal-wlr &
fi
AUTO
        log_ok "Added portal startup to autostart"
    fi
fi

print_summary "Portal Setup" "ok" "xdg-desktop-portal-wlr installed. Screen sharing should work in Wayland."
