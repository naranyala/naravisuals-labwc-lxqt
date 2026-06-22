#!/bin/bash
# Setup Fedora/RHEL Repositories
# Enables EPEL, CRB/PowerTools, and necessary Copr repos
# for building/installing LXQt and its dependencies.
#
# Usage:
#   bash setup-fedora-repos.sh              # Interactive
#   bash setup-fedora-repos.sh --all        # Enable everything
#   bash setup-fedora-repos.sh --dry-run    # Preview only

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

DRY_RUN=false
FORCE=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        --all|-a) FORCE=true ;;
        --help|-h)
            printf "Setup Fedora/RHEL Repositories\n\n"
            printf "Usage: bash setup-fedora-repos.sh [options]\n\n"
            printf "Options:\n"
            printf "  --all, -f        Enable all repos without prompting\n"
            printf "  --dry-run        Preview without changes\n"
            printf "  --help, -h       Show this help\n"
            exit 0
            ;;
    esac
done

DISTRO="$(detect_distro)"
VERSION="$(detect_distro_version)"

print_header "Fedora/RHEL Repository Setup"

if [ "$DISTRO" = "debian" ] || [ "$DISTRO" = "arch" ]; then
    log_info "This script is for Fedora/RHEL-based systems."
    log_info "Detected: $DISTRO"
    log_info "On Debian/Ubuntu, use: sudo apt-get install -y <package>"
    log_info "On Arch, use: sudo pacman -S <package>"
    exit 0
fi

log_info "Detected: $DISTRO $VERSION"

# ---- 1. EPEL (Extra Packages for Enterprise Linux) ----
log_step "EPEL Repository"
if [ "$DISTRO" = "rhel" ] || [ "$DISTRO" = "centos" ] || [ "$DISTRO" = "rocky" ] || [ "$DISTRO" = "alma" ]; then
    if rpm -q epel-release &>/dev/null; then
        log_ok "EPEL already installed"
    else
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would install: epel-release"
        else
            sudo dnf install -y epel-release 2>/dev/null && log_ok "EPEL installed" || log_warn "Failed to install EPEL"
        fi
    fi
elif [ "$DISTRO" = "fedora" ]; then
    log_ok "Fedora has EPEL-compatible packages built-in, skipping"
fi

# ---- 2. CRB / PowerTools (needed for some -devel packages) ----
log_step "CRB / PowerTools Repository"
if [ "$DISTRO" = "fedora" ]; then
    log_info "Fedora does not use CRB/PowerTools, skipping"
elif command -v dnf &>/dev/null; then
    # RHEL 9+ / CentOS Stream 9+ / Rocky 9+ use crb
    # RHEL 8 / CentOS 8 use powertools
    if sudo dnf config-manager --set-enabled crb 2>/dev/null; then
        log_ok "Enabled CRB"
    elif sudo dnf config-manager --set-enabled powertools 2>/dev/null; then
        log_ok "Enabled PowerTools"
    else
        log_warn "Could not enable CRB/PowerTools (may not be needed)"
    fi
fi

# ---- 3. Fedora Copr Repos ----
log_step "Fedora Copr Repositories"
if [ "$DISTRO" = "fedora" ] && cmd_exists "dnf"; then
    # labwc may need Copr on older Fedora versions
    enable_copr "zhsj/labwc" || true

    # Waybar latest if not in repos
    if ! pkg_available "waybar" 2>/dev/null; then
        enable_copr "klassiker/worldclock" || true
    fi
fi

# ---- 4. Verify key packages are available ----
log_step "Verifying Package Availability"

PACKAGES=(
    "cmake"
    "ninja-build"
    "git"
    "labwc"
    "swaybg"
    "swayidle"
    "swaylock"
    "dunst"
    "sddm"
    "wl-clipboard"
    "rofi"
    "grim"
    "slurp"
    "qt6-qtbase-devel"
    "kf6-kwindowsystem-devel"
    "polkit-qt6-1-devel"
)

AVAILABLE=0
MISSING=()

for pkg in "${PACKAGES[@]}"; do
    if pkg_available "$pkg" 2>/dev/null; then
        AVAILABLE=$((AVAILABLE + 1))
    else
        MISSING+=("$pkg")
    fi
done

log_info "Available: $AVAILABLE / ${#PACKAGES[@]} packages"

if [ ${#MISSING[@]} -gt 0 ]; then
    log_warn "Missing packages:"
    for pkg in "${MISSING[@]}"; do
        printf "  ${YELLOW}•${RST} %s\n" "$pkg"
    done
    echo ""
    log_info "These may need Copr repos, manual installation, or building from source."
    log_info "Run: bash scripts/build/build_latest_lxqt_desktop.sh to build from source."
fi

# ---- Summary ----
print_summary "Repo Setup" "ok" "Fedora/RHEL repos configured. Run the build script next."
