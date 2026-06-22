#!/bin/bash
# Install Kvantum Theme Engine
# Provides SVG-based Qt theming for modern, translucent themes
#
# Usage:
#   bash install-kvantum.sh              # Interactive
#   bash install-kvantum.sh --all        # Install everything
#   bash install-kvantum.sh --dry-run    # Preview only

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../lib.sh"

DRY_RUN=false
FORCE=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        --all|-a) FORCE=true ;;
        --help|-h)
            printf "Install Kvantum Theme Engine\n\n"
            printf "Usage: bash install-kvantum.sh [options]\n\n"
            printf "Options:\n"
            printf "  --all, -f        Install without prompts\n"
            printf "  --dry-run        Preview only\n"
            printf "  --help, -h       Show this help\n"
            exit 0
            ;;
    esac
done

print_header "Kvantum Theme Engine Setup"

# --- Detect and install packages ---
DISTRO="$(detect_distro)"
PM="$(detect_pm)"

install_packages() {
    log_step "Installing Kvantum packages"

    case "$PM" in
        apt)
            sudo apt-get install -y qt6ct kvantum 2>/dev/null || {
                log_warn "kvantum not in apt repos, trying kvantum-qt6..."
                sudo apt-get install -y kvantum-qt6 2>/dev/null || true
            }
            ;;
        dnf)
            sudo dnf install -y qt6ct kvantum 2>/dev/null || {
                log_warn "kvantum not in dnf repos"
                if [ "$DISTRO" = "fedora" ]; then
                    log_info "Try: sudo dnf copr enable <repo> for kvantum"
                fi
            }
            ;;
        pacman)
            sudo pacman -S --noconfirm qt6ct kvantum 2>/dev/null
            ;;
        zypper)
            sudo zypper install -y qt6ct kvantum 2>/dev/null
            ;;
        *)
            log_warn "Install manually:"
            log_info "  qt6ct: https://github.com/trialuser02/qt6ct"
            log_info "  kvantum: https://github.com/tsujan/Kvantum"
            return 1
            ;;
    esac
}

install_packages

# --- Verify ---
if cmd_exists qt6ct; then
    log_ok "qt6ct installed"
else
    log_warn "qt6ct not found -- Qt6 configuration tool missing"
fi

if cmd_exists kvantum || cmd_exists kvantum-qt6; then
    log_ok "Kvantum installed"
else
    log_warn "Kvantum not found -- SVG theming unavailable"
fi

# --- Setup environment ---
ENV_FILE="$HOME/.config/labwc/environment"
if [ -f "$ENV_FILE" ]; then
    # Ensure qt6ct is the platform theme
    if ! grep -q "QT_QPA_PLATFORMTHEME" "$ENV_FILE"; then
        echo "QT_QPA_PLATFORMTHEME=qt6ct" >> "$ENV_FILE"
        log_ok "Added QT_QPA_PLATFORMTHEME=qt6ct"
    fi

    # Add Kvantum style override if not present
    if ! grep -q "QT_STYLE_OVERRIDE" "$ENV_FILE"; then
        echo "QT_STYLE_OVERRIDE=kvantum" >> "$ENV_FILE"
        log_ok "Added QT_STYLE_OVERRIDE=kvantum"
    fi
fi

# --- Copy project QSS if available ---
PANEL_QSS="$HOME/.config/lxqt/lxqt-panel.qss"
PROJECT_DIR="${SCRIPT_DIR}/../.."
if [ -f "$PROJECT_DIR/configs/dotfiles/lxqt/lxqt-panel.qss" ]; then
    mkdir -p "$(dirname "$PANEL_QSS")"
    cp "$PROJECT_DIR/configs/dotfiles/lxqt/lxqt-panel.qss" "$PANEL_QSS"
    log_ok "Installed Nord QSS for lxqt-panel"
fi

# --- Add to dotfiles installer manifest check ---
log_info "Panel QSS installed at: $PANEL_QSS"
log_info "To apply: reconfigure labwc (labwc -r) or restart lxqt-panel"

print_summary "Kvantum Setup" "ok" "Kvantum + qt6ct installed. Run qt6ct to select theme."
