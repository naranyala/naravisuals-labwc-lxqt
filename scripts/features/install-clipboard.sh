#!/bin/bash
# Install Clipboard Manager (wl-clipboard + cliphist)
# Source lib.sh for shared utilities

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

print_header "Clipboard Manager Setup"

# --- Detect and install packages ---
PM="$(detect_pm)"

install_packages() {
    case "$PM" in
        apt)
            sudo apt-get install -y wl-clipboard 2>/dev/null
            # cliphist may need to be built from source on Debian
            if ! cmd_exists cliphist; then
                log_info "cliphist not in repos, attempting cargo install..."
                if cmd_exists cargo; then
                    cargo install cliphist
                else
                    log_warn "Install cargo/rust to build cliphist, or install manually"
                    log_info "  https://github.com/sentriz/cliphist"
                fi
            fi
            ;;
        dnf)
            sudo dnf install -y wl-clipboard 2>/dev/null
            if ! cmd_exists cliphist; then
                log_info "cliphist not in repos, attempting cargo install..."
                if cmd_exists cargo; then
                    cargo install cliphist
                else
                    log_warn "Install cargo/rust to build cliphist"
                fi
            fi
            ;;
        pacman)
            sudo pacman -S --noconfirm wl-clipboard cliphist 2>/dev/null
            ;;
        *)
            log_warn "Unknown package manager. Install manually:"
            log_info "  wl-clipboard: https://github.com/bugaevc/wl-clipboard"
            log_info "  cliphist: https://github.com/sentriz/cliphist"
            return 1
            ;;
    esac
}

install_packages

# --- Verify ---
if cmd_exists wl-copy && cmd_exists wl-paste; then
    log_ok "wl-clipboard installed"
else
    log_error "wl-clipboard not found after install"
    return 1 2>/dev/null || exit 1
fi

if cmd_exists cliphist; then
    log_ok "cliphist installed"
else
    log_warn "cliphist not found — install manually for clipboard history"
fi

# --- Setup config ---
CLIPHIST_DIR="$HOME/.config/cliphist"
mkdir -p "$CLIPHIST_DIR"

# Copy our config if it exists in the project
PROJECT_DIR="${SCRIPT_DIR}/../.."
if [ -f "$PROJECT_DIR/configs/dotfiles/cliphist/config" ]; then
    cp "$PROJECT_DIR/configs/dotfiles/cliphist/config" "$CLIPHIST_DIR/config" 2>/dev/null
fi

# --- Add to labwc autostart if not present ---
AUTOSTART="$HOME/.config/labwc/autostart"
if [ -f "$AUTOSTART" ]; then
    if ! grep -q "cliphist" "$AUTOSTART"; then
        log_info "Adding cliphist to labwc autostart..."
        cat >> "$AUTOSTART" << 'AUTO'

# Clipboard history (add after wl-clipboard section)
if command -v cliphist &>/dev/null && command -v wl-paste &>/dev/null; then
  wl-paste --type text --watch cliphist store &
  wl-paste --type image --watch cliphist store &
fi
AUTO
        log_ok "Added to autostart"
    fi
fi

print_summary "Clipboard Setup" "ok" "wl-clipboard + cliphist ready. Use W-Shift-V to open history."
