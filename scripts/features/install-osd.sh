#!/bin/bash
# Install OSD (On-Screen Display) for volume/brightness feedback
# Uses wob — a lightweight Wayland overlay bar

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

print_header "OSD Setup (wob)"

# --- Detect and install ---
PM="$(detect_pm)"

install_packages() {
    case "$PM" in
        apt)
            sudo apt-get install -y wob 2>/dev/null || {
                log_warn "wob not in apt repos, building from source..."
                build_wob
            }
            ;;
        dnf)
            sudo dnf install -y wob 2>/dev/null || {
                log_warn "wob not in dnf repos, building from source..."
                build_wob
            }
            ;;
        pacman)
            sudo pacman -S --noconfirm wob 2>/dev/null
            ;;
        *)
            log_info "Building wob from source..."
            build_wob
            ;;
    esac
}

build_wob() {
    if ! cmd_exists cargo; then
        log_error "cargo/rust required to build wob"
        log_info "Install: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
        return 1
    fi

    local tmpdir
    tmpdir="$(mktemp -d)"
    log_info "Cloning wob..."
    git_clone "https://github.com/francma/wob.git" "$tmpdir/wob"
    cd "$tmpdir/wob" || return 1
    cargo build --release 2>/dev/null
    sudo cp target/release/wob /usr/local/bin/
    cd "$OLDPWD"
    rm -rf "$tmpdir"
    log_ok "wob built and installed"
}

install_packages

# --- Verify ---
if cmd_exists wob; then
    log_ok "wob installed: $(wob --version 2>/dev/null || echo 'ok')"
else
    log_error "wob not found after install"
    return 1 2>/dev/null || exit 1
fi

# --- Create wob FIFO ---
WOB_FIFO="/tmp/wob_fifo"
if [ ! -p "$WOB_FIFO" ]; then
    mkfifo "$WOB_FIFO" 2>/dev/null
    log_ok "Created wob FIFO at $WOB_FIFO"
fi

# --- Setup config ---
WOB_DIR="$HOME/.config/wob"
mkdir -p "$WOB_DIR"

# Copy project config if available
PROJECT_DIR="${SCRIPT_DIR}/../.."
if [ -f "$PROJECT_DIR/configs/dotfiles/wob/config" ]; then
    cp "$PROJECT_DIR/configs/dotfiles/wob/config" "$WOB_DIR/config"
    log_ok "Installed wob config"
fi

# --- Add wob startup to labwc autostart ---
AUTOSTART="$HOME/.config/labwc/autostart"
if [ -f "$AUTOSTART" ]; then
    if ! grep -q "wob_fifo" "$AUTOSTART"; then
        log_info "Adding wob to labwc autostart..."
        cat >> "$AUTOSTART" << 'AUTO'

# OSD overlay bar (wob)
if command -v wob &>/dev/null; then
  WOB_FIFO="/tmp/wob_fifo"
  [ -p "$WOB_FIFO" ] || mkfifo "$WOB_FIFO"
  tail -f "$WOB_FIFO" | wob &
fi
AUTO
        log_ok "Added wob to autostart"
    fi
fi

# --- Patch rc.xml volume/brightness keys to pipe through wob ---
RC_XML="$HOME/.config/labwc/rc.xml"
if [ -f "$RC_XML" ] && ! grep -q "wob" "$RC_XML"; then
    log_info "Patching rc.xml to use wob OSD feedback..."

    # Volume up
    sed -i 's|command="pactl set-sink-volume @DEFAULT_SINK@ +5%"|command="pactl set-sink-volume @DEFAULT_SINK@ +5% \&\& pactl get-sink-volume @DEFAULT_SINK@ | grep -oP '\''\d+%'\'' | head -1 > /tmp/wob_fifo"|g' "$RC_XML"

    # Volume down
    sed -i 's|command="pactl set-sink-volume @DEFAULT_SINK@ -5%"|command="pactl set-sink-volume @DEFAULT_SINK@ -5% \&\& pactl get-sink-volume @DEFAULT_SINK@ | grep -oP '\''\d+%'\'' | head -1 > /tmp/wob_fifo"|g' "$RC_XML"

    # Brightness up
    sed -i 's|command="brightnessctl set +5%"|command="brightnessctl set +5% \&\& brightnessctl -m | cut -d, -f4 > /tmp/wob_fifo"|g' "$RC_XML"

    # Brightness down
    sed -i 's|command="brightnessctl set 5%-"|command="brightnessctl set 5%- \&\& brightnessctl -m | cut -d, -f4 > /tmp/wob_fifo"|g' "$RC_XML"

    log_ok "rc.xml patched with OSD feedback"
fi

print_summary "OSD Setup" "ok" "wob installed. Volume/brightness changes will show an overlay bar."
