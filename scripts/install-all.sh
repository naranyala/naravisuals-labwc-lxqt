#!/bin/bash
# LXQt Rice — Full Resource Installer
# ======================================
# Composable orchestrator that runs all download scripts
# in sequence. Each script is independent and can be
# run separately, but this provides a unified flow.
#
# Usage:
#   bash install-all.sh               # Interactive (ask for each)
#   bash install-all.sh --all         # Install everything
#   bash install-all.sh --select      # Choose which to run
#   bash install-all.sh --dry-run     # Preview only
#   bash install-all.sh themes icons  # Run specific modules only
#
# Design: each sub-script is sourced, not exec'd, so they
# share the same environment and library.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# ---- Available Modules ----
# Format: "SCRIPT|NAME|DESCRIPTION"
MODULES=(
  "themes.sh|Theme Downloader|GTK/Qt window themes (Nordic, Dracula, Catppuccin...)"
  "icons.sh|Icon Themes|Desktop icon packs (Tela, Papirus, Candy...)"
  "cursors.sh|Cursor Themes|Mouse cursor themes (Bibata, Capitaine, Nordic...)"
  "fonts.sh|Font Installer|Nerd Fonts & UI fonts (JetBrains Mono, Fira Code...)"
  "wallpapers.sh|Wallpaper Packs|Wallpaper collections (Nord, Catppuccin, Gruvbox...)"
  "labwc-themes.sh|labwc Decorations|Window decoration themes (Vent, Nordic, Dracula...)"
  "lxqt-themes.sh|LXQt Widget Themes|Qt palette themes for lxqt-config-appearance"
  "neofetch.sh|System Fetch|Install & configure neofetch/fastfetch/hyfetch"
  "conky.sh|Conky Desktop|Conky system monitor configs"
  "emacs.sh|Emacs Editor|Install Emacs GUI + minimal beginner-friendly config"
)

run_module() {
  local script="$1" name="$2" desc="$3"

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

  if [ "$DRY_RUN" = "true" ]; then
    log_info "[DRY-RUN] Would run: bash $script"
    return 0
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
  printf "${BOLD}%-20s %s${RST}\n" "MODULE" "STATUS"
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────────────"
  for entry in "${RESULTS[@]}"; do
    IFS='|' read -r name status <<< "$entry"
    local icon="${GREEN}✔${RST}"
    [ "$status" = "fail" ] && icon="${RED}✘${RST}"
    [ "$status" = "skip" ] && icon="${YELLOW}–${RST}"
    printf "  %-20s %b\n" "$name" "$icon"
  done
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────────────"
}

show_menu() {
  printf "\n${BOLD}Select modules to install:${RST}\n"
  printf "${DIM}  Enter numbers separated by space (e.g., '1 2 3')${RST}\n"
  printf "${DIM}  Enter 'a' for all, 'q' to quit${RST}\n\n"

  local i=1
  for entry in "${MODULES[@]}"; do
    IFS='|' read -r _ name desc <<< "$entry"
    printf "  ${BOLD}%2d)${RST} %-20s ${DIM}%s${RST}\n" "$i" "$name" "$desc"
    i=$((i + 1))
  done
  printf "  ${BOLD} a)${RST} ${GREEN}Install All${RST}\n"
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
declare -a SELECTED
declare -a RESULTS

# Parse args
for arg in "$@"; do
  case "$arg" in
    --all|-a) RUN_ALL=true ;;
    --dry-run) DRY_RUN=true ;;
    --interactive|-i) INTERACTIVE=true ;;
    --select) show_menu ;;
    --help|-h)
      printf "LXQt Rice — Full Resource Installer\n\n"
      printf "Usage: bash install-all.sh [options] [modules...]\n\n"
      printf "Options:\n"
      printf "  --all, -a        Install all modules\n"
      printf "  --select         Show interactive menu\n"
      printf "  --interactive    Ask before each module\n"
      printf "  --dry-run        Preview without downloading\n"
      printf "  --help, -h       Show this help\n\n"
      printf "Modules: themes, icons, cursors, fonts, wallpapers,\n"
      printf "         labwc-themes, lxqt-themes, neofetch, conky\n"
      exit 0
      ;;
    *)
      # Map module names to indices (must handle set -u)
      i=0
      for entry in "${MODULES[@]}"; do
        IFS='|' read -r script name _ <<< "$entry"
        basename="${script%.sh}"
        if [ "$arg" = "$basename" ] || [ "$arg" = "$name" ]; then
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
  # Either --all was passed, or no specific modules selected
  RUN_ALL=true
fi

if [ "$RUN_ALL" != true ] && [ "$sel_count" -eq 0 ]; then
  # No selection made, show menu
  show_menu
fi

# Run modules
total=${#MODULES[@]}
ok_count=0
fail_count=0
skip_count=0

for ((i = 0; i < total; i++)); do
  IFS='|' read -r script name desc <<< "${MODULES[$i]}"

  if [ "$RUN_ALL" = true ] || [ "${SELECTED[$i]:-false}" = true ]; then
    if run_module "$script" "$name" "$desc"; then
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
