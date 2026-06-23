#!/bin/bash
# LXQt Rice — Full Resource Installer
# ======================================
# Composable orchestrator that runs all download scripts
# in sequence with dependency ordering.
#
# Usage:
#   bash install-all.sh               # Interactive (ask for each)
#   bash install-all.sh --all         # Install everything
#   bash install-all.sh --select      # Choose which to run
#   bash install-all.sh --dry-run     # Preview only
#   bash install-all.sh themes icons  # Run specific modules only
#
# Module ordering respects dependencies:
#   1. System setup (repos, portals, fontconfig)
#   2. Core tools (clipboard, OSD, launcher, kvantum)
#   3. Visual resources (themes, icons, cursors, fonts, wallpapers)
#   4. Optional extras (neofetch, conky, emacs)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ---- Module Definitions ----
# Format: "SCRIPT|NAME|DESCRIPTION|GROUP"
# GROUP determines ordering and dependency resolution
MODULES=(
  # Group 1: System setup (run first)
  "features/setup-fedora-repos.sh|Fedora/RHEL Repos|Enable EPEL, CRB, Copr repos|system"
  "features/setup-fontconfig.sh|Font Rendering|fontconfig for text rendering|system"
  "features/setup-portals.sh|XDG Portals|Screen sharing & file dialogs|system"

  # Group 2: Core tools
  "features/install-clipboard.sh|Clipboard Manager|wl-clipboard + cliphist|tools"
  "features/install-osd.sh|OSD Overlay|wob for volume/brightness|tools"
  "features/install-launcher.sh|App Launcher|rofi-wayland|tools"
  "features/install-kvantum.sh|Kvantum Themes|Qt theme engine + qt6ct|tools"
  "features/wallust-setup.sh|Dynamic Theming|Wallust color extraction|tools"

  # Group 3: Compositor (optional, user chooses)
  "features/install-compositor.sh hyprland|Hyprland|Animated tiling compositor|compositor"
  "features/install-compositor.sh sway|Sway|i3-compatible tiling|compositor"
  "features/install-compositor.sh wayfire|Wayfire|3D plugin compositor|compositor"

  # Group 4: Visual resources (download order doesn't matter much)
  "themes.sh|Theme Downloader|GTK/Qt window themes|visual"
  "icons.sh|Icon Themes|Desktop icon packs|visual"
  "cursors.sh|Cursor Themes|Mouse cursor themes|visual"
  "fonts.sh|Font Installer|Nerd Fonts & UI fonts|visual"
  "wallpapers.sh|Wallpaper Packs|Wallpaper collections|visual"
  "labwc-themes.sh|labwc Decorations|Window decoration themes|visual"
  "lxqt-themes.sh|LXQt Widget Themes|Qt palette themes|visual"

  # Group 5: Optional extras
  "neofetch.sh|System Fetch|neofetch/fastfetch/hyfetch|extras"
  "conky.sh|Conky Desktop|Conky system monitor|extras"
  "emacs.sh|Emacs Editor|Emacs GUI + config|extras"
  "install-minimal-apps.sh --both|Minimal Apps|Qt + GTK equivalent apps|extras"
)

# Module dependencies: KEY=module_index, VALUE=space-separated indices that must run first
# This ensures fontconfig runs before fonts, etc.
declare -A DEPENDS_ON=()
# fontconfig (index 1) must run before fonts (index 14)
DEPENDS_ON[14]="1"

run_module() {
  local idx="$1" script="$2" name="$3" desc="$4"

  printf "\n"
  printf "${BOLD}${MAGENTA}╔══════════════════════════════════════════╗${RST}\n"
  printf "${BOLD}${MAGENTA}║${RST}  ${BOLD}[${GREEN}%s${RST}]${BOLD} %s${RST}\n" "$name" "$desc"
  printf "${BOLD}${MAGENTA}╚══════════════════════════════════════════╝${RST}\n"

  if [ "${INTERACTIVE}" = "true" ]; then
    printf "${BOLD}Run this module? [Y/n] ${RST}"
    read -r input
    case "$input" in
      n|N|no|NO) log_dim "skipping $name"; return 0 ;;
      *) ;;
    esac
  fi

  if [ "$DRY_RUN" = true ]; then
    log_info "[DRY-RUN] Would run: bash $script"
    return 0
  fi

  # Check dependencies
  if [ -n "${DEPENDS_ON[$idx]:-}" ]; then
    for dep_idx in ${DEPENDS_ON[$idx]}; do
      if [ "${RESULTS[$dep_idx]:-skip}" != "ok" ]; then
        local dep_name
        IFS='|' read -r _ dep_name _ _ <<< "${MODULES[$dep_idx]}"
        log_warn "Dependency not met: $dep_name must run successfully first"
        return 1
      fi
    done
  fi

  log_step "Starting: $name"
  if bash "$SCRIPT_DIR/$script" --all --force 2>&1 | sed 's/^/  /'; then
    log_ok "Completed: $name"
    return 0
  else
    log_warn "Module had issues: $name"
    return 1
  fi
}

print_banner() {
  clear 2>/dev/null || true
  printf "${BOLD}${MAGENTA}"
  printf "╔══════════════════════════════════════════════════════════╗\n"
  printf "║                                                          ║\n"
  printf "║              LXQt Desktop Rice Setup                     ║\n"
  printf "║       Download external resources for full theme         ║\n"
  printf "║                                                          ║\n"
  printf "╚══════════════════════════════════════════════════════════╝\n"
  printf "${RST}\n"
}

print_summary_table() {
  printf "\n${BOLD}${CYAN}Installation Summary${RST}\n"
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────────────"
  printf "${BOLD}%-20s %-10s %s${RST}\n" "MODULE" "STATUS" "GROUP"
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────────────"
  for i in "${!RESULTS[@]}"; do
    IFS='|' read -r name status <<< "${RESULTS[$i]}"
    IFS='|' read -r _ _ _ group <<< "${MODULES[$i]}"
    local icon="${GREEN}✔${RST}"
    [ "$status" = "fail" ] && icon="${RED}✘${RST}"
    [ "$status" = "skip" ] && icon="${YELLOW}–${RST}"
    printf "  %-20s %b %-10s\n" "$name" "$icon" "$group"
  done
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────────────"
}

show_menu() {
  printf "\n${BOLD}Select modules to install:${RST}\n"
  printf "${DIM}  Enter numbers separated by space (e.g., '1 2 3')${RST}\n"
  printf "${DIM}  Enter 'a' for all, 'q' to quit${RST}\n\n"

  local prev_group=""
  local i=1
  for entry in "${MODULES[@]}"; do
    IFS='|' read -r _ name desc group <<< "$entry"
    if [ "$group" != "$prev_group" ]; then
      printf "\n  ${BOLD}${CYAN}── %s ──${RST}\n" "${group^^}"
      prev_group="$group"
    fi
    printf "  ${BOLD}%2d)${RST} %-20s ${DIM}%s${RST}\n" "$i" "$name" "$desc"
    i=$((i + 1))
  done
  printf "\n  ${BOLD} a)${RST} ${GREEN}Install All${RST}\n"
  printf "  ${BOLD} q)${RST} Quit\n"
  printf "\n${BOLD}Selection: ${RST}"
  read -r selection

  case "$selection" in
    q|Q) exit 0 ;;
    a|A) RUN_ALL=true ;;
    *)
      for num in $selection; do
        local idx=$((num - 1))
        if [ "$idx" -ge 0 ] && [ "$idx" -lt ${#MODULES[@]} ]; then
          SELECTED[$idx]=true
        fi
      done
      ;;
  esac
}

# ---- Main ----
INTERACTIVE=false
DRY_RUN=false
RUN_ALL=false
declare -a SELECTED=()
declare -a RESULTS=()

# Parse args
for arg in "$@"; do
  case "$arg" in
    --all|-a) RUN_ALL=true ;;
    --dry-run) DRY_RUN=true ;;
    --interactive|-i) INTERACTIVE=true ;;
    --select) show_menu ;;
    --force|-f) ;; # Accept but ignore (sub-scripts handle it)
    --help|-h)
      printf "LXQt Rice — Full Resource Installer\n\n"
      printf "Usage: bash install-all.sh [options] [modules...]\n\n"
      printf "Options:\n"
      printf "  --all, -a        Install all modules\n"
      printf "  --select         Show interactive menu\n"
      printf "  --interactive    Ask before each module\n"
      printf "  --dry-run        Preview without downloading\n"
      printf "  --help, -h       Show this help\n\n"
      printf "Modules are grouped by dependency:\n"
      printf "  system:   repos, fontconfig, portals\n"
      printf "  tools:    clipboard, OSD, launcher, kvantum\n"
      printf "  compositor: hyprland, sway, wayfire\n"
      printf "  visual:   themes, icons, cursors, fonts, wallpapers\n"
      printf "  extras:   neofetch, conky, emacs\n"
      exit 0
      ;;
    *)
      # Map module names to indices
      i=0
      for entry in "${MODULES[@]}"; do
        IFS='|' read -r script name _ _ <<< "$entry"
        mod_name="${script%.sh}"
        if [ "$arg" = "$mod_name" ] || [ "$arg" = "$name" ]; then
          SELECTED[$i]=true
        fi
        i=$((i + 1))
      done
      ;;
  esac
done

print_banner

# Check for required tools
require_cmds curl tar unzip find || log_warn "Some tools missing; modules may fail"

# Determine what to run
set +u
sel_count=${#SELECTED[@]}
set -u

if [ "$RUN_ALL" = true ] || [ "$sel_count" -eq 0 ]; then
  RUN_ALL=true
fi

if [ "$RUN_ALL" != true ] && [ "$sel_count" -eq 0 ]; then
  show_menu
fi

# Run modules in order (dependency-aware)
total=${#MODULES[@]}
ok_count=0
fail_count=0
skip_count=0

for ((i = 0; i < total; i++)); do
  IFS='|' read -r script name desc group <<< "${MODULES[$i]}"

  if [ "$RUN_ALL" = true ] || [ "${SELECTED[$i]:-false}" = true ]; then
    if run_module "$i" "$script" "$name" "$desc"; then
      RESULTS+=("$name|ok")
      ok_count=$((ok_count + 1))
    else
      RESULTS+=("$name|fail")
      fail_count=$((fail_count + 1))
    fi
  else
    RESULTS+=("$name|skip")
    skip_count=$((skip_count + 1))
  fi
done

# Print final summary
print_summary_table

printf "\n"
printf "${BOLD}${GREEN}╔══════════════════════════════════════════╗${RST}\n"
printf "${BOLD}${GREEN}║${RST}  ${BOLD}Total: %d  |  OK: %d  |  Fail: %d  |  Skip: %d${RST}\n" "$total" "$ok_count" "$fail_count" "$skip_count"
printf "${BOLD}${GREEN}╚══════════════════════════════════════════╝${RST}\n"
printf "\n"
printf "${BOLD}Next steps:${RST}\n"
printf "  1. Apply themes: ${GREEN}lxqt-config-appearance${RST}\n"
printf "  2. Set icons:     ${GREEN}lxqt-config-appearance${RST} → Icon Theme\n"
printf "  3. Set cursor:    ${GREEN}lxqt-config-appearance${RST} → Cursor Theme\n"
printf "  4. Set fonts:     ${GREEN}lxqt-config-appearance${RST} → Font\n"
printf "  5. Set wallpaper: ${GREEN}bash wallpapers.sh --set <collection>${RST}\n"
printf "  6. Reconfigure:   ${GREEN}labwc -r${RST}\n"

[ "$fail_count" -eq 0 ] || exit 1
