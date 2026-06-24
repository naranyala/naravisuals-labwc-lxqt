#!/bin/bash
# Install and configure a Wayland compositor for LXQt
# Supports: labwc, wayfire, kwin_wayland (stacking)
#           Hyprland, sway, niri, river (tiling)
#           miriway (Mir-based stacking)
#
# Usage:
#   bash install-compositor.sh <compositor>
#   bash install-compositor.sh --list

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

COMPOSITOR=""
DRY_RUN=false

# Map: compositor -> package_name (empty = from-source)
declare -A PKG_MAP=(
    [hyprland]="hyprland"
    [sway]="sway"
    [wayfire]="wayfire"
    [labwc]="labwc"
    [niri]="niri"
    [river]="river"
    [kwin_wayland]="kwin"
    [miriway]=""
)

# Map: compositor -> display name for messages
declare -A NAME_MAP=(
    [hyprland]="Hyprland"
    [sway]="Sway"
    [wayfire]="Wayfire"
    [labwc]="Labwc"
    [niri]="Niri"
    [river]="River"
    [kwin_wayland]="KWin-Wayland"
    [miriway]="Miriway"
)

# Map: compositor -> session.conf compositor value
declare -A CONF_MAP=(
    [hyprland]="Hyprland"
    [sway]="sway"
    [wayfire]="wayfire"
    [labwc]="labwc"
    [niri]="niri"
    [river]="river"
    [kwin_wayland]="kwin_wayland"
    [miriway]="miriway"
)

# Map: compositor -> type for description
declare -A TYPE_MAP=(
    [hyprland]="tiling"
    [sway]="tiling"
    [wayfire]="stacking"
    [labwc]="stacking"
    [niri]="tiling"
    [river]="tiling"
    [kwin_wayland]="stacking"
    [miriway]="stacking"
)

ALL_COMPOSITORS=(hyprland sway wayfire labwc niri river kwin_wayland miriway)

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --list)
            printf "Available compositor profiles:\n"
            for c in "${ALL_COMPOSITORS[@]}"; do
                pkg="${PKG_MAP[$c]}"
                src=""
                [ -z "$pkg" ] && src=" (build from source)"
                printf "  %-14s %s%s\n" "$c" "${NAME_MAP[$c]}" "$src"
            done
            exit 0
            ;;
        --help|-h)
            printf "Install Wayland Compositor for LXQt\n\n"
            printf "Usage: bash install-compositor.sh <compositor>\n\n"
            printf "Compositors:\n"
            for c in "${ALL_COMPOSITORS[@]}"; do
                printf "  %-14s %s (%s)\n" "$c" "${NAME_MAP[$c]}" "${TYPE_MAP[$c]}"
                pkg="${PKG_MAP[$c]}"
                [ -n "$pkg" ] && printf "  %-14s   Package: %s\n" "" "$pkg" \
                    || printf "  %-14s   Build from source\n" ""
            done
            printf "\nOptions:\n"
            printf "  --list     List available profiles\n"
            printf "  --dry-run  Preview only\n"
            exit 0
            ;;
        hyprland|sway|wayfire|labwc|niri|river|kwin_wayland|miriway)
            COMPOSITOR="$arg"
            ;;
    esac
done

if [ -z "$COMPOSITOR" ]; then
    log_error "Specify a compositor: ${ALL_COMPOSITORS[*]}"
    log_info "Run with --list to see options"
    exit 1
fi

print_header "Install Compositor: ${NAME_MAP[$COMPOSITOR]}"

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

log_step "Installing ${NAME_MAP[$COMPOSITOR]}"
case "$COMPOSITOR" in
    miriway)
        log_info "Miriway must be built from source: bash scripts/update-mir-stack.sh"
        ;;
    *)
        install_pkg "${PKG_MAP[$COMPOSITOR]}" || true
        ;;
esac

# --- Copy compositor config ---
PROJECT_DIR="${SCRIPT_DIR}/../.."
SRC_DIR="$PROJECT_DIR/configs/compositors/$COMPOSITOR"
XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"

config_installed=false
if [ -d "$SRC_DIR" ]; then
    case "$COMPOSITOR" in
        hyprland)
            mkdir -p "$XDG_CONFIG_HOME/hypr"
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/hyprland.conf -> $XDG_CONFIG_HOME/hypr/"
            else
                cp "$SRC_DIR/hyprland.conf" "$XDG_CONFIG_HOME/hypr/"
                log_ok "Installed Hyprland config"
                config_installed=true
            fi
            ;;
        sway)
            mkdir -p "$XDG_CONFIG_HOME/sway"
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/config -> $XDG_CONFIG_HOME/sway/"
            else
                cp "$SRC_DIR/config" "$XDG_CONFIG_HOME/sway/"
                chmod 600 "$XDG_CONFIG_HOME/sway/config"
                log_ok "Installed Sway config"
                config_installed=true
            fi
            ;;
        wayfire)
            mkdir -p "$XDG_CONFIG_HOME/wayfire"
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/wayfire.ini -> $XDG_CONFIG_HOME/wayfire/"
            else
                cp "$SRC_DIR/wayfire.ini" "$XDG_CONFIG_HOME/wayfire/"
                log_ok "Installed Wayfire config"
                config_installed=true
            fi
            ;;
        labwc)
            log_info "Labwc configs already handled by dotfiles installer"
            config_installed=true
            ;;
        niri)
            mkdir -p "$XDG_CONFIG_HOME/lxqt/wayland"
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/lxqt-niri.kdl -> $XDG_CONFIG_HOME/lxqt/wayland/"
            else
                cp "$SRC_DIR/lxqt-niri.kdl" "$XDG_CONFIG_HOME/lxqt/wayland/"
                log_ok "Installed Niri config"
                config_installed=true
            fi
            ;;
        river)
            mkdir -p "$XDG_CONFIG_HOME/lxqt/wayland"
            if [ "$DRY_RUN" = true ]; then
                log_dim "[DRY-RUN] Would copy $SRC_DIR/lxqt-river-init -> $XDG_CONFIG_HOME/lxqt/wayland/"
            else
                cp "$SRC_DIR/lxqt-river-init" "$XDG_CONFIG_HOME/lxqt/wayland/"
                chmod 755 "$XDG_CONFIG_HOME/lxqt/wayland/lxqt-river-init"
                log_ok "Installed River config"
                config_installed=true
            fi
            ;;
        kwin_wayland)
            log_info "KWin is configured via Plasma System Settings GUI"
            config_installed=true
            ;;
        miriway)
            MIRIWAY_CONFIG="$XDG_CONFIG_HOME/miriway-shell.config"
            if [ ! -f "$MIRIWAY_CONFIG" ]; then
                DEFAULT_CONFIG="$SRC_DIR/miriway-shell.config"
                if [ -f "$DEFAULT_CONFIG" ]; then
                    if [ "$DRY_RUN" = true ]; then
                        log_dim "[DRY-RUN] Would copy $DEFAULT_CONFIG -> $MIRIWAY_CONFIG"
                    else
                        cp "$DEFAULT_CONFIG" "$MIRIWAY_CONFIG"
                        log_ok "Installed Miriway config"
                        config_installed=true
                    fi
                fi
            else
                log_info "Miriway config already exists at $MIRIWAY_CONFIG"
                config_installed=true
            fi
            ;;
    esac
fi

# --- Update session.conf ---
SESSION_CONF="$XDG_CONFIG_HOME/lxqt/session.conf"
if [ -f "$SESSION_CONF" ]; then
    log_step "Updating session.conf"
    conf_val="${CONF_MAP[$COMPOSITOR]}"
    sed -i 's/^window_manager=.*/window_manager=/' "$SESSION_CONF" 2>/dev/null || true
    sed -i "s/^compositor=.*/compositor=$conf_val/" "$SESSION_CONF" 2>/dev/null || \
        echo "compositor=$conf_val" >> "$SESSION_CONF"
    log_ok "Session manager configured for ${NAME_MAP[$COMPOSITOR]}"
fi

# --- Summary ---
print_summary "Compositor Install" "ok" "${NAME_MAP[$COMPOSITOR]} installed and configured"
printf "\n${BOLD}Next steps:${RST}\n"
printf "  1. Log out of current session\n"
printf "  2. Select 'LXQt (${NAME_MAP[$COMPOSITOR]})' from your display manager\n"
