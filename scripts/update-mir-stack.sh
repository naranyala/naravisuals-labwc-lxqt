#!/bin/bash
# Update, build, and install mir + miracle-wm + miriway from inspirations/
# Usage: bash scripts/update-mir-stack.sh [--clean]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_ROOT="$PROJECT_ROOT/build"
INSTALL_CMD="pkexec"

MIR_SRC="$PROJECT_ROOT/inspirations/canonical__mir"
MIRACLE_SRC="$PROJECT_ROOT/inspirations/miracle-wm-org__miracle-wm"
MIRIWAY_SRC="$PROJECT_ROOT/inspirations/Miriway__Miriway"

MIR_BUILD="$BUILD_ROOT/mir"
MIRACLE_BUILD="$BUILD_ROOT/miracle-wm"
MIRIWAY_BUILD="$BUILD_ROOT/miriway"

NPROC="$(nproc)"
CLEAN_BUILD=false

[[ "${1:-}" == "--clean" ]] && CLEAN_BUILD=true

update_repo() {
    local dir="$1" name="$2"
    log_step "Updating $name"
    if [[ -d "$dir/.git" ]]; then
        git -C "$dir" fetch --tags --prune --force
        local branch
        branch=$(git -C "$dir" rev-parse --abbrev-ref HEAD)
        git -C "$dir" pull origin "$branch"
        log_ok "$name updated ($(git -C "$dir" rev-parse --short HEAD))"
    else
        log_warn "$name has no .git — skipping update"
    fi
}

configure_and_build() {
    local src="$1" build="$2" name="$3" cmake_opts="$4"
    log_step "Configuring $name"
    if $CLEAN_BUILD; then
        rm -rf "$build"
        mkdir -p "$build"
    fi
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr $cmake_opts \
        -S "$src" -B "$build" 2>&1 | tail -5
    log_ok "$name configured"

    log_step "Building $name"
    ninja -C "$build" -j"$NPROC" 2>&1 | tail -5
    log_ok "$name built"
}

install_project() {
    local build="$1" name="$2"
    log_step "Installing $name"
    $INSTALL_CMD ninja -C "$build" install 2>&1 | tail -5
    log_ok "$name installed"
}

# ---- Mir ----
log_info "Mir: $MIR_SRC -> $MIR_BUILD"
update_repo "$MIR_SRC" "mir"
configure_and_build "$MIR_SRC" "$MIR_BUILD" "mir" \
    "-DMIR_ENABLE_TESTS=OFF -DMIR_ENABLE_RUST=ON -DMIR_PLATFORM=gbm-kms;x11;wayland"
install_project "$MIR_BUILD" "mir"

# ---- miracle-wm (depends on mir) ----
log_info "miracle-wm: $MIRACLE_SRC -> $MIRACLE_BUILD"
update_repo "$MIRACLE_SRC" "miracle-wm"
configure_and_build "$MIRACLE_SRC" "$MIRACLE_BUILD" "miracle-wm" \
    "-DCOMPILING_AGAINST_DEV=ON -DENABLE_TESTS=OFF -DFEATURE_PLUGIN_SYSTEM=ON -DBUILD_ERROR_REPORTER=ON -DBUILD_DEBUG_OVERLAY=ON -DSYSTEMD_INTEGRATION=OFF"
install_project "$MIRACLE_BUILD" "miracle-wm"

# ---- Miriway (depends on mir) ----
log_info "miriway: $MIRIWAY_SRC -> $MIRIWAY_BUILD"
update_repo "$MIRIWAY_SRC" "miriway"
configure_and_build "$MIRIWAY_SRC" "$MIRIWAY_BUILD" "miriway" "-DSDDM=OFF"
install_project "$MIRIWAY_BUILD" "miriway"

log_step "All mir-stack packages updated, built, and installed"
log_info "Mir version:  $(pkg-config --modversion mirserver)"
log_info "miral version: $(pkg-config --modversion miral)"
log_info "miracle-wm:    $(which miracle-wm)"
log_info "miriway:       $(which miriway-shell)"
