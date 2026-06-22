#!/bin/bash
# Install and configure a Wayland compositor for LXQt
# Supports: Hyprland, Sway, Wayfire (Labwc is the default, handled separately)
#
# Usage:
#   bash install-compositor.sh hyprland
#   bash install-compositor.sh sway
#   bash install-compositor.sh wayfire
#   bash install-compositor.sh --list

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

COMPOSITOR=""
DRY_RUN=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --list)
            printf "Available compositor profiles:\n"
            printf "  hyprland   Hyprland (animations, blur, tiling)\n"
            printf "  sway       Sway (i3-compatible tiling)\n"
            printf "  wayfire    Wayfire (3D compositor, plugin-based)\n"
            printf "  labwc      Labwc (default, Openbox-like stacking)\n"
            exit 0
            ;;
        --help|-h)
            printf "Install Wayland Compositor for LXQt\n\n"
            printf "Usage: bash install-compositor.sh <compositor>\n\n"
            printf "Compositors:\n"
            printf "  hyprland   Animated tiling compositor\n"
            printf "  sway       i3-compatible tiling compositor\n"
            printf "  wayfire    3D plugin-based compositor\n"
            printf "  labwc      Openbox-like stacking (default)\n\n"
            printf "Options:\n"
            printf "  --list     List available profiles\n"
            printf "  --dry-run  Preview only\n"
            exit 0
            ;;
        hyprland|sway|wayfire|labwc)
            COMPOSITOR="$arg"
            ;;
    esac
done

if [ -z "$COMPOSITOR" ]; then
    log_error "Specify a compositor: hyprland, sway, wayfire, or labwc"
    log_info "Run with --list to see options"
    exit 1
fi

print_header "Install Compositor: $COMPOSITOR"

# --- Install compositor package ---
install_pkg() {
    local pkg="$1"
    if pkg_available "$pkg"; then
        pkg_install "$pkg"
    else
        log_warn "Package not available: $pkg"
        log_info "Check your distro repos or install manually"
        return 1
    fi
}

log_step "Installing $COMPOSITOR"
case "$COMPOSITOR" in
    hyprland)
        install_pkg "hyprland" || {
            log_info "Hyprland may need to be built from source"
            log_info "  https://wiki.hyprland.org/Getting-Started/Installation/"
        }
        ;;
    sway)
        install_pkg "sway" ;;
    wayfire)
        install_pkg "wayfire" ;;
    labwc)
        install_pkg "labwc" ;;
esac

# --- Copy compositor config ---
COMPOSITOR_DIR="$HOME/.config/$COMPOSITOR"
PROJECT_DIR="${SCRIPT_DIR}/../.."
SRC_DIR="$PROJECT_DIR/configs/compositors/$COMPOSITOR"

if [ -d "$SRC_DIR" ]; then
    mkdir -p "$COMPOSITOR_DIR"

    case "$COMPOSITOR" in
        hyprland)
            mkdir -p "$HOME/.config/hypr"
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/hyprland.conf -> $HOME/.config/hypr/"
            else
                cp "$SRC_DIR/hyprland.conf" "$HOME/.config/hypr/"
                log_ok "Installed Hyprland config"
            fi
            ;;
        sway)
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/config -> $COMPOSITOR_DIR/"
            else
                cp "$SRC_DIR/config" "$COMPOSITOR_DIR/"
                chmod 600 "$COMPOSITOR_DIR/config"
                log_ok "Installed Sway config"
            fi
            ;;
        wayfire)
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/wayfire.ini -> $COMPOSITOR_DIR/"
            else
                mkdir -p "$COMPOSITOR_DIR"
                cp "$SRC_DIR/wayfire.ini" "$COMPOSITOR_DIR/"
                log_ok "Installed Wayfire config"
            fi
            ;;
        labwc)
            log_info "Labwc is the default. Configs already in ~/.config/labwc/"
            ;;
    esac
fi

# --- Update session.conf ---
SESSION_CONF="$HOME/.config/lxqt/session.conf"
if [ -f "$SESSION_CONF" ]; then
    log_step "Updating session.conf"
    case "$COMPOSITOR" in
        hyprland)
            sed -i 's/^window_manager=.*/window_manager=Hyprland/' "$SESSION_CONF" 2>/dev/null || \
                echo "window_manager=Hyprland" >> "$SESSION_CONF"
            ;;
        sway)
            sed -i 's/^window_manager=.*/window_manager=sway/' "$SESSION_CONF" 2>/dev/null || \
                echo "window_manager=sway" >> "$SESSION_CONF"
            ;;
        wayfire)
            sed -i 's/^window_manager=.*/window_manager=wayfire/' "$SESSION_CONF" 2>/dev/null || \
                echo "window_manager=wayfire" >> "$SESSION_CONF"
            ;;
        labwc)
            sed -i 's/^window_manager=.*/window_manager=labwc/' "$SESSION_CONF" 2>/dev/null || \
                echo "window_manager=labwc" >> "$SESSION_CONF"
            ;;
    esac
    log_ok "Session manager configured for $COMPOSITOR"
fi

# --- Summary ---
print_summary "Compositor Install" "ok" "$COMPOSITOR installed and configured"
printf "\n${BOLD}Next steps:${RST}\n"
printf "  1. Log out of current session\n"
printf "  2. Select '%s' from your display manager\n" "$COMPOSITOR"
printf "  3. Or launch manually: %s\n" "$COMPOSITOR"
