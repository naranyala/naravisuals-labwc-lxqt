#!/bin/bash
# Naravisuals — Build LXQt Components from Source
# ==================================================
# Builds LXQt components from the lxqt/ submodule tree.
# Respects dependency order so each component's libraries
# are installed before the next one that needs them.
#
# Usage:
#   bash build-lxqt-from-source.sh              # --essential (default)
#   bash build-lxqt-from-source.sh --full        # All 33 components
#   bash build-lxqt-from-source.sh --list        # List components & exit
#   bash build-lxqt-from-source.sh --prefix /opt/lxqt
#   bash build-lxqt-from-source.sh --dry-run     # Preview only
#   bash build-lxqt-from-source.sh --help        # This message

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LXQT_SRC="$(cd "$SCRIPT_DIR/../lxqt" && pwd)"
PREFIX="${PREFIX:-/usr/local}"
MODE="essential"
DRY_RUN=false
JOBS=$(nproc 2>/dev/null || echo 4)
GENERATOR=""
MAKE_CMD=""

# ─── Logging ──────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[0;33m'; BLUE='\033[0;34m'
BOLD='\033[1m'; DIM='\033[2m'; RST='\033[0m'
log()    { printf "${BLUE}::${RST} %s\n" "$*"; }
ok()     { printf "${GREEN}  OK${RST} %s\n" "$*"; }
warn()   { printf "${YELLOW}  !${RST} %s\n" "$*" >&2; }
dim()    { printf "${DIM}    %s${RST}\n" "$*"; }

# ─── Parse args ──────────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --full|-a)   MODE="full" ;;
        --essential) MODE="essential" ;;
        --list)      MODE="list" ;;
        --dry-run)   DRY_RUN=true ;;
        --prefix=*)  PREFIX="${arg#--prefix=}" ;;
        --help|-h)
            sed -n '3,16p' "$0" | sed 's/^# \?//'
            exit 0
            ;;
    esac
done

# ─── Dependency-ordered component lists ───────────────────────────────────────
# Grouped by build tier — each tier depends on all previous tiers.
# Format: "dir_name|description"

TIER0=(
    "lxqt-build-tools|CMake build tooling (must be first)"
)
TIER1=(
    "lxqt-menu-data|FreeDesktop menu data"
    "libqtxdg|Qt XDG implementation"
)
TIER2=(
    "liblxqt|Core LXQt library"
    "menu-cache|Menu cache library (autotools)"
)
TIER3=(
    "libsysstat|System statistics library"
    "libdbusmenu-lxqt|D-Bus menu integration"
)
TIER4=(
    "libfm-qt|File manager library"
)
TIER5=(
    "lxqt-themes|LXQt theme files"
    "lxqt-qtplugin|Qt platform integration plugin"
)
TIER6=(
    "lxqt-session|Session manager"
    "lxqt-globalkeys|Global keyboard shortcuts"
    "lxqt-notificationd|Notification daemon"
    "lxqt-policykit|PolicyKit authentication agent"
)
TIER7=(
    "lxqt-powermanagement|Power management"
    "lxqt-runner|Application runner (Alt+F2)"
    "lxqt-config|Configuration center"
    "lxqt-about|About dialog"
    "lxqt-admin|Administration tools"
    "lxqt-openssh-askpass|SSH password prompt"
)
TIER8=(
    "lxqt-panel|Desktop panel"
    "pcmanfm-qt|File manager"
    "lxqt-sudo|Graphical sudo frontend"
    "pavucontrol-qt|Audio volume control"
    "lxqt-archiver|Archive manager"
    "qtxdg-tools|XDG command-line tools"
    "xdg-desktop-portal-lxqt|XDG file portal backend"
    "lxqt-wayland-session|Wayland session files"
)
TIER9=(
    "qtermwidget|Terminal widget library"
    "lximage-qt|Image viewer"
    "obconf-qt|Openbox config tool (not needed for labwc)"
    "qps|Process viewer"
    "screengrab|Screenshot tool"
)
TIER10=(
    "qterminal|Terminal emulator (depends on qtermwidget)"
)

# ─── Display component tree ──────────────────────────────────────────────────
if [ "$MODE" = "list" ]; then
    echo "LXQt components in dependency order:"
    echo ""
    for tier_name in TIER0 TIER1 TIER2 TIER3 TIER4 TIER5 TIER6 TIER7 TIER8 TIER9 TIER10; do
        declare -n tier="$tier_name"
        echo "  Tier ${tier_name#TIER}:"
        for entry in "${tier[@]}"; do
            dir="${entry%%|*}"
            desc="${entry#*|}"
            if [ -d "$LXQT_SRC/$dir" ]; then
                printf "    ${GREEN}%-30s${RST} %s\n" "$dir" "$desc"
            else
                printf "    ${RED}%-30s${RST} MISSING from lxqt/\n" "$dir"
            fi
        done
        echo ""
    done
    echo "LXQT_SRC=$LXQT_SRC"
    echo "PREFIX=$PREFIX"
    exit 0
fi

# ─── Select which tiers to build ─────────────────────────────────────────────
SELECTED_TIERS=()
case "$MODE" in
    essential)
        # Essential: the minimum to run an LXQt desktop
        SELECTED_TIERS=(TIER0 TIER1 TIER2 TIER3 TIER4 TIER5 TIER6 TIER7 TIER8)
        ;;
    full)
        SELECTED_TIERS=(TIER0 TIER1 TIER2 TIER3 TIER4 TIER5 TIER6 TIER7 TIER8 TIER9 TIER10)
        ;;
esac

# ─── Detect build tool ────────────────────────────────────────────────────────
if command -v ninja &>/dev/null; then
    GENERATOR="-GNinja"
    MAKE_CMD="ninja"
    dim "Using Ninja ($(ninja --version 2>/dev/null))"
elif command -v make &>/dev/null; then
    GENERATOR=""
    MAKE_CMD="make"
    dim "Using Make"
else
    warn "Neither ninja nor make found!"
    exit 1
fi

# ─── Pre-flight: check source exists ──────────────────────────────────────────
if [ ! -d "$LXQT_SRC" ]; then
    warn "lxqt/ source directory not found at $LXQT_SRC"
    warn "Make sure git submodules are initialized:"
    warn "  cd lxqt && git submodule init && git submodule update --remote"
    exit 1
fi

# ─── Summary ──────────────────────────────────────────────────────────────────
printf "\n${BOLD}Building LXQt from source${RST}\n"
printf "  Source:    ${BOLD}%s${RST}\n" "$LXQT_SRC"
printf "  Prefix:    ${BOLD}%s${RST}\n" "$PREFIX"
printf "  Mode:      ${BOLD}%s${RST}\n" "$MODE"
printf "  Jobs:      ${BOLD}%s${RST}\n" "$JOBS"
printf "  Generator: ${BOLD}%s${RST}\n" "${GENERATOR:-Makefiles}"
printf "\n"

ALL_CMAKE_FLAGS=(
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_INSTALL_PREFIX=$PREFIX"
    "-DCMAKE_PREFIX_PATH=$PREFIX"
    $GENERATOR
)

build_component() {
    local dir="$1"
    local desc="$2"
    local src_path="$LXQT_SRC/$dir"
    local build_path="$src_path/build"

    if [ ! -d "$src_path" ]; then
        warn "Source missing: $dir — skipping"
        return 1
    fi

    log "Building: $dir ($desc)"

    if [ "$DRY_RUN" = true ]; then
        dim "  mkdir -p $build_path"
        dim "  cd $build_path && cmake ${ALL_CMAKE_FLAGS[*]} .. && $MAKE_CMD -j$JOBS && sudo $MAKE_CMD install"
        return 0
    fi

    # Handle autotools (menu-cache)
    local is_autotools=false
    [ -f "$src_path/configure.ac" ] && [ ! -f "$src_path/CMakeLists.txt" ] && is_autotools=true

    if [ "$is_autotools" = true ]; then
        dim "  (autotools build)"
        cd "$src_path"
        if [ ! -f "configure" ]; then
            ./autogen.sh 2>/dev/null || autoreconf -fi 2>/dev/null || true
        fi
        mkdir -p build && cd build
        ../configure --prefix="$PREFIX" --quiet && make -j"$JOBS" && sudo make install
        cd "$LXQT_SRC"
        ok "$dir installed"
        return 0
    fi

    # CMake build
    mkdir -p "$build_path"
    cd "$build_path"
    if cmake "${ALL_CMAKE_FLAGS[@]}" "$src_path" 2>&1 | tail -1; then
        if $MAKE_CMD -j"$JOBS" 2>&1 | tail -3; then
            if sudo $MAKE_CMD install 2>&1 | tail -1; then
                cd "$LXQT_SRC"
                ok "$dir installed"
                return 0
            fi
        fi
    fi
    warn "$dir build failed — check output above"
    cd "$LXQT_SRC"
    return 1
}

# ─── Build ────────────────────────────────────────────────────────────────────
BUILT=0
FAILED=0
SKIPPED=0

for tier_name in "${SELECTED_TIERS[@]}"; do
    declare -n tier="$tier_name"
    tier_num="${tier_name#TIER}"

    if [ "$DRY_RUN" = false ]; then
        printf "\n${BOLD}── Tier %s ──${RST}\n" "$tier_num"
    fi

    for entry in "${tier[@]}"; do
        dir="${entry%%|*}"
        desc="${entry#*|}"
        if [ ! -d "$LXQT_SRC/$dir" ]; then
            warn "Source not found: lxqt/$dir — skip"
            SKIPPED=$((SKIPPED + 1))
            continue
        fi
        if build_component "$dir" "$desc"; then
            BUILT=$((BUILT + 1))
        else
            FAILED=$((FAILED + 1))
        fi
    done
done

# ─── Summary ──────────────────────────────────────────────────────────────────
printf "\n"
printf "${BOLD}Build complete:${RST} %d built, %d failed, %d skipped\n" \
    "$BUILT" "$FAILED" "$SKIPPED"
printf "\n"

if [ "$FAILED" -gt 0 ]; then
    printf "${YELLOW}Troubleshooting:${RST}\n"
    printf "  - Missing dependencies? Install: sudo apt build-dep lxqt\n"
    printf "  - Qt6 not found? Install: sudo apt install qt6-base-dev qt6-tools-dev\n"
    printf "  - Run again: bash build-lxqt-from-source.sh --full\n"
fi

printf "${DIM}Note: You still need these from distro packages:${RST}\n"
printf "${DIM}  sudo apt install lxqt-session lxqt-panel pcmanfm-qt qterminal${RST}\n"
printf "${DIM}  (unless you built them from source above)${RST}\n"

exit $FAILED
