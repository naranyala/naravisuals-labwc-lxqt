#!/bin/bash
# Install Application Launcher (rofi-wayland)
# Provides a modern launcher for Labwc + LXQt

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../lib.sh"

print_header "Application Launcher Setup"

# --- Detect and install ---
PM="$(detect_pm)"

install_packages() {
    case "$PM" in
        apt)
            sudo apt-get install -y rofi 2>/dev/null
            # For Wayland support, rofi-wayland fork may be needed
            if ! rofi -v 2>&1 | grep -qi wayland; then
                log_info "Standard rofi installed. For native Wayland, build rofi-wayland fork:"
                log_info "  https://github.com/lbonn/rofi"
            fi
            ;;
        dnf)
            sudo dnf install -y rofi 2>/dev/null
            ;;
        pacman)
            sudo pacman -S --noconfirm rofi-wayland 2>/dev/null || sudo pacman -S --noconfirm rofi 2>/dev/null
            ;;
        *)
            log_warn "Install rofi manually: https://github.com/davatorium/rofi"
            return 1
            ;;
    esac
}

install_packages

# --- Verify ---
if cmd_exists rofi; then
    log_ok "rofi installed"
else
    log_error "rofi not found after install"
    return 1 2>/dev/null || exit 1
fi

# --- Setup config ---
ROFI_DIR="$HOME/.config/rofi"
mkdir -p "$ROFI_DIR"

# Copy project config if available
PROJECT_DIR="${SCRIPT_DIR}/../.."
if [ -f "$PROJECT_DIR/configs/dotfiles/rofi/config.rasi" ]; then
    cp "$PROJECT_DIR/configs/dotfiles/rofi/config.rasi" "$ROFI_DIR/config.rasi"
    log_ok "Installed rofi config (Nord theme)"
fi

# --- Update rc.xml launcher keybinding ---
RC_XML="$HOME/.config/labwc/rc.xml"
if [ -f "$RC_XML" ]; then
    if grep -q 'command="lxqt-runner"' "$RC_XML"; then
        log_info "Replacing lxqt-runner with rofi in rc.xml..."
        sed -i 's|command="lxqt-runner"|command="rofi -show drun"|g' "$RC_XML"
        log_ok "Launcher keybinding updated to rofi"
    fi
fi

print_summary "Launcher Setup" "ok" "rofi installed with Nord theme. Alt+F2 / Super+D to launch."
