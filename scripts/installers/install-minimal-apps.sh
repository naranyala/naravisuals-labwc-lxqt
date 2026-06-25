#!/bin/bash
# Naravisuals — Minimal Apps Installer (Qt + GTK Equivalents)
# =============================================================
# Install minimal Qt-based apps alongside their lightweight
# GTK-based equivalents from distribution repositories.
#
# Usage:
#   bash install-minimal-apps.sh                # Interactive
#   bash install-minimal-apps.sh --qt-only      # Qt apps only
#   bash install-minimal-apps.sh --gtk-only     # GTK equivalents only
#   bash install-minimal-apps.sh --both         # Install both
#   bash install-minimal-apps.sh --select       # Pick categories
#   bash install-minimal-apps.sh --dry-run      # Preview only
#   bash install-minimal-apps.sh --list         # Show app mapping table
#   bash install-minimal-apps.sh --remove-gtk   # Uninstall GTK apps
#   bash install-minimal-apps.sh --remove-qt    # Uninstall Qt apps

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

# ---- Defaults ----
MODE="interactive"
DRY_RUN="${DRY_RUN:-false}"
FORCE="${FORCE:-false}"
TOOLKIT=""  # "qt", "gtk", "both"

for arg in "$@"; do
    case "$arg" in
        --qt-only|-q)    TOOLKIT="qt" ;;
        --gtk-only|-g)   TOOLKIT="gtk" ;;
        --both|-b)       TOOLKIT="both" ;;
        --select|-s)     MODE="select" ;;
        --dry-run|-n)    DRY_RUN=true ;;
        --list|-l)       MODE="list" ;;
        --remove-gtk)    MODE="remove-gtk" ;;
        --remove-qt)     MODE="remove-qt" ;;
        --help|-h)
            printf "Naravisuals Minimal Apps Installer (Qt + GTK)\n\n"
            printf "Usage: bash install-minimal-apps.sh [options]\n\n"
            printf "Options:\n"
            printf "  --qt-only,  -q   Install Qt apps only\n"
            printf "  --gtk-only, -g   Install GTK equivalents only\n"
            printf "  --both,     -b   Install both Qt and GTK apps\n"
            printf "  --select,   -s   Choose categories interactively\n"
            printf "  --dry-run,  -n   Preview without installing\n"
            printf "  --list,     -l   Show Qt ↔ GTK mapping table\n"
            printf "  --remove-gtk     Uninstall GTK equivalents\n"
            printf "  --remove-qt      Uninstall Qt apps\n"
            printf "  --help,     -h   Show this help\n"
            exit 0
            ;;
    esac
done

# ═══════════════════════════════════════════════════════════════
# APP MAPPING TABLE
# ═══════════════════════════════════════════════════════════════
# Format: "CATEGORY|DESCRIPTION|QT_PACKAGES|GTK_PACKAGES"
#
# QT_PACKAGES / GTK_PACKAGES: comma-separated with fallbacks
#   using "/" for alternatives (first available wins).
#
# Package names use distro-aware fallback logic via lib.sh's
# pkg_install_fallback function.
# ═══════════════════════════════════════════════════════════════

APPS=(
  # ── Audio ──
  "audio|Audio / Volume Control|pavucontrol-qt|pavucontrol"

  # ── Display ──
  "display|Display / Monitor Settings|lxqt-config-monitor/lxqt-config|arandr/wdisplays/gnome-control-center"

  # ── Network ──
  "network|Network / WiFi Manager|qconnman-ui/connman-qt|nm-connection-editor/network-manager-gnome/connman-gtk"

  # ── Bluetooth ──
  "bluetooth|Bluetooth Manager|bluedevil/qbluez|blueman/gnome-bluetooth"

  # ── Power ──
  "power|Power Management|lxqt-powermanagement/lxqt-config|xfce4-power-manager/gnome-power-manager/mate-power-manager"

  # ── Input ──
  "input|Input Devices (kbd/mouse/touch)|lxqt-config-input/lxqt-config|xinput/xfce4-settings"

  # ── Disk ──
  "disk|Disk / Partition Info|partitionmanager/kde-partitionmanager|gnome-disk-utility/gparted/baobab"

  # ── Theme ──
  "theme|Theme / Appearance|qt6ct/kvantum-qt6|lxappearance/nwg-look"

  # ── File Manager ──
  "filemanager|File Manager|pcmanfm-qt|pcmanfm/thunar/nautilus/nemo/caja"

  # ── Terminal ──
  "terminal|Terminal Emulator|qterminal|xfce4-terminal/mate-terminal/lxterminal/alacritty/foot"

  # ── Text Editor ──
  "editor|Text Editor|featherpad/kate|mousepad/xed/pluma/gedit/l3afpad"

  # ── Image Viewer ──
  "imageviewer|Image Viewer|lximage-qt/nomacs|eog/eom/ristretto/feh/imv"

  # ── PDF Viewer ──
  "pdf|PDF / Document Viewer|qpdfview/okular|evince/atril/zathura"

  # ── Archive Manager ──
  "archive|Archive Manager|lxqt-archiver/ark|file-roller/engrampa/xarchiver"

  # ── Screenshot ──
  "screenshot|Screenshot Tool|spectacle/lxqt-screenshot|gnome-screenshot/xfce4-screenshooter/flameshot"

  # ── System Monitor ──
  "sysmon|System / Task Monitor|qps/lxqt-admin|gnome-system-monitor/mate-system-monitor/xfce4-taskmanager/htop"

  # ── Notification ──
  "notification|Notification Daemon|lxqt-notificationd|dunst/mako/swaync"

  # ── Wallpaper ──
  "wallpaper|Wallpaper Setter|nitrogen uses qt internally/swaybg|nitrogen/swaybg/feh"

  # ── Clipboard ──
  "clipboard|Clipboard Manager|qlipper/copyq|parcellite/clipit/gpaste"

  # ── App Launcher ──
  "launcher|Application Launcher|lxqt-runner|rofi-wayland/rofi/dmenu/wofi/fuzzel/bemenu"

  # ── Lock Screen ──
  "lockscreen|Lock Screen|light-locker|swaylock/swaylock-effects/gtklock/i3lock"

  # ── Polkit ──
  "polkit|Polkit Auth Agent|lxqt-policykit|polkit-gnome/mate-polkit/xfce-polkit"

  # ── Session ──
  "session|Session Manager|lxqt-session|xfce4-session/mate-session-manager"
)

# ═══════════════════════════════════════════════════════════════
# CATEGORIES FOR --select MODE
# ═══════════════════════════════════════════════════════════════
CATEGORIES=(
    "essential|Essential Desktop (file mgr, terminal, editor, polkit, session)"
    "multimedia|Multimedia (audio, image viewer, screenshot)"
    "system|System Utils (disk, sysmon, power, input, display)"
    "network|Connectivity (network, bluetooth)"
    "appearance|Appearance (theme, wallpaper, notification)"
    "productivity|Productivity (pdf, archive, clipboard, launcher, lockscreen)"
)

# Map category -> app categories
declare -A CAT_MAP=(
    [essential]="filemanager terminal editor polkit session"
    [multimedia]="audio imageviewer screenshot"
    [system]="disk sysmon power input display"
    [network]="network bluetooth"
    [appearance]="theme wallpaper notification"
    [productivity]="pdf archive clipboard launcher lockscreen"
)

# ═══════════════════════════════════════════════════════════════
# FUNCTIONS
# ═══════════════════════════════════════════════════════════════

# Parse an app entry
parse_app() {
    local entry="$1"
    APP_CAT="${entry%%|*}";   entry="${entry#*|}"
    APP_DESC="${entry%%|*}";  entry="${entry#*|}"
    APP_QT="${entry%%|*}";    entry="${entry#*|}"
    APP_GTK="$entry"
}

# Print the mapping table
print_mapping_table() {
    printf "\n"
    printf "${BOLD}${CYAN}%-14s %-28s %-35s %-35s${RST}\n" \
        "Category" "Description" "Qt Package(s)" "GTK Package(s)"
    printf "${DIM}%s${RST}\n" \
        "────────────── ──────────────────────────── ─────────────────────────────────── ───────────────────────────────────"

    for app in "${APPS[@]}"; do
        parse_app "$app"
        # Clean up fallback notation for display
        local qt_display="${APP_QT//\// | }"
        local gtk_display="${APP_GTK//\// | }"
        printf "  ${GREEN}%-12s${RST} %-28s ${BLUE}%-35s${RST} ${YELLOW}%-35s${RST}\n" \
            "$APP_CAT" "$APP_DESC" "$qt_display" "$gtk_display"
    done
    printf "\n"
    printf "${DIM}Fallback order: first available package in each slot is installed.${RST}\n"
    printf "${DIM}Use --qt-only or --gtk-only to install a single toolkit set.${RST}\n\n"
}

# Install packages for a single app entry
install_app_packages() {
    local toolkit="$1"
    local entry="$2"
    parse_app "$entry"

    local packages_str
    case "$toolkit" in
        qt)  packages_str="$APP_QT" ;;
        gtk) packages_str="$APP_GTK" ;;
    esac

    # Convert slash-separated fallbacks to array
    local IFS='/'
    local fallbacks=()
    read -ra fallbacks <<< "$packages_str"

    if [ "$DRY_RUN" = true ]; then
        log_dim "  [DRY-RUN] Would install ($toolkit): ${fallbacks[*]}"
        return 0
    fi

    # Try each fallback in order
    for pkg in "${fallbacks[@]}"; do
        # Trim whitespace
        pkg="$(echo "$pkg" | xargs)"
        [ -z "$pkg" ] && continue

        if is_installed "$pkg" 2>/dev/null; then
            log_ok "$APP_DESC: $pkg (already installed)"
            return 0
        fi
    done

    # None installed, try to install the first available
    for pkg in "${fallbacks[@]}"; do
        pkg="$(echo "$pkg" | xargs)"
        [ -z "$pkg" ] && continue

        if pkg_available "$pkg" 2>/dev/null; then
            log_info "Installing $APP_DESC ($toolkit): $pkg"
            if pkg_install "$pkg" 2>/dev/null; then
                log_ok "$APP_DESC: $pkg installed"
                return 0
            fi
        fi
    done

    log_warn "$APP_DESC ($toolkit): no package available — ${fallbacks[*]}"
    return 1
}

# Remove packages for a single app entry
remove_app_packages() {
    local toolkit="$1"
    local entry="$2"
    parse_app "$entry"

    local packages_str
    case "$toolkit" in
        qt)  packages_str="$APP_QT" ;;
        gtk) packages_str="$APP_GTK" ;;
    esac

    local IFS='/'
    local fallbacks=()
    read -ra fallbacks <<< "$packages_str"

    for pkg in "${fallbacks[@]}"; do
        pkg="$(echo "$pkg" | xargs)"
        [ -z "$pkg" ] && continue

        if is_installed "$pkg" 2>/dev/null; then
            if [ "$DRY_RUN" = true ]; then
                log_dim "  [DRY-RUN] Would remove: $pkg"
            else
                log_info "Removing: $pkg"
                local pm
                pm="$(detect_pm)"
                case "$pm" in
                    apt)    sudo apt-get remove -y "$pkg" 2>/dev/null ;;
                    dnf)    sudo dnf remove -y "$pkg" 2>/dev/null ;;
                    pacman) sudo pacman -Rns --noconfirm "$pkg" 2>/dev/null ;;
                    zypper) sudo zypper remove -y "$pkg" 2>/dev/null ;;
                esac
                log_ok "Removed: $pkg"
            fi
        fi
    done
}

# Install all apps for a toolkit
install_all() {
    local toolkit="$1"
    local filter_cats="${2:-}"  # space-separated categories to install, empty = all
    local ok=0 fail=0 skip=0

    local tk_label
    case "$toolkit" in
        qt)  tk_label="${BLUE}Qt${RST}" ;;
        gtk) tk_label="${YELLOW}GTK${RST}" ;;
    esac

    log_step "Installing $tk_label apps"

    for app in "${APPS[@]}"; do
        parse_app "$app"

        # Filter by categories if specified
        if [ -n "$filter_cats" ]; then
            local found=false
            for cat in $filter_cats; do
                if [ "$cat" = "$APP_CAT" ]; then
                    found=true
                    break
                fi
            done
            [ "$found" = false ] && continue
        fi

        if install_app_packages "$toolkit" "$app"; then
            ok=$((ok + 1))
        else
            fail=$((fail + 1))
        fi
    done

    printf "\n"
    log_info "Results: ${GREEN}$ok installed${RST}, ${RED}$fail unavailable${RST}"
}

# ═══════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════

# ---- List mode ----
if [ "$MODE" = "list" ]; then
    print_header "Naravisuals App Mapping — Qt ↔ GTK"
    print_mapping_table
    exit 0
fi

# ---- Remove modes ----
if [ "$MODE" = "remove-gtk" ] || [ "$MODE" = "remove-qt" ]; then
    local_tk="${MODE#remove-}"
    print_header "Removing $local_tk apps"

    if [ "$DRY_RUN" != true ]; then
        confirm "Remove all installed $local_tk equivalent apps?" || exit 0
    fi

    for app in "${APPS[@]}"; do
        remove_app_packages "$local_tk" "$app"
    done

    print_summary "Removal ($local_tk)" "ok"
    exit 0
fi

# ---- Install modes ----
print_header "Naravisuals Minimal Apps Installer"

printf "\n"
printf "${DIM}Detected: $(detect_distro) / $(detect_pm)${RST}\n"
printf "\n"

# Resolve toolkit choice
if [ -z "$TOOLKIT" ]; then
    if [ "$MODE" = "interactive" ]; then
        printf "${BOLD}Which toolkit set do you want to install?${RST}\n"
        printf "  ${GREEN}1${RST}) Qt apps only       (lightweight Qt/LXQt apps)\n"
        printf "  ${GREEN}2${RST}) GTK apps only      (lightweight GTK equivalents)\n"
        printf "  ${GREEN}3${RST}) Both               (Qt + GTK side by side)\n"
        printf "\n"
        printf "${BOLD}Choice [1/2/3]: ${RST}"
        read -r choice
        case "$choice" in
            1) TOOLKIT="qt" ;;
            2) TOOLKIT="gtk" ;;
            3) TOOLKIT="both" ;;
            *) TOOLKIT="both" ;;
        esac
    else
        TOOLKIT="both"
    fi
fi

# Resolve categories for --select mode
FILTER_CATS=""
if [ "$MODE" = "select" ]; then
    printf "\n${BOLD}Select categories to install:${RST}\n\n"
    local_selected=()
    for i in "${!CATEGORIES[@]}"; do
        local entry="${CATEGORIES[$i]}"
        local cat_id="${entry%%|*}"
        local cat_desc="${entry#*|}"
        printf "  ${GREEN}%d${RST}) %s\n" "$((i + 1))" "$cat_desc"
    done

    printf "\n${BOLD}Enter numbers (space-separated, e.g. '1 3 5') or 'all': ${RST}"
    read -r selections

    if [ "$selections" = "all" ] || [ -z "$selections" ]; then
        FILTER_CATS=""
    else
        for sel in $selections; do
            idx=$((sel - 1))
            if [ "$idx" -ge 0 ] && [ "$idx" -lt "${#CATEGORIES[@]}" ]; then
                local entry="${CATEGORIES[$idx]}"
                local cat_id="${entry%%|*}"
                local app_cats="${CAT_MAP[$cat_id]}"
                FILTER_CATS="$FILTER_CATS $app_cats"
            fi
        done
        FILTER_CATS="$(echo "$FILTER_CATS" | xargs)"
    fi
fi

# ---- Run installation ----
case "$TOOLKIT" in
    qt)
        install_all "qt" "$FILTER_CATS"
        ;;
    gtk)
        install_all "gtk" "$FILTER_CATS"
        ;;
    both)
        install_all "qt" "$FILTER_CATS"
        install_all "gtk" "$FILTER_CATS"
        ;;
esac

# ---- Summary ----
printf "\n"
print_summary "Minimal Apps Installation" "ok"
printf "\n"
printf "${BOLD}Installed toolkit:${RST} %s\n" "$TOOLKIT"
printf "\n"
printf "${BOLD}Quick reference:${RST}\n"
printf "  View mapping:   ${GREEN}bash %s --list${RST}\n" "$(basename "$0")"
printf "  Remove GTK:     ${GREEN}bash %s --remove-gtk${RST}\n" "$(basename "$0")"
printf "  Remove Qt:      ${GREEN}bash %s --remove-qt${RST}\n" "$(basename "$0")"
printf "  Dry run:        ${GREEN}bash %s --both --dry-run${RST}\n" "$(basename "$0")"
printf "\n"
