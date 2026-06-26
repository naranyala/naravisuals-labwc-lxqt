#!/bin/bash
# Naravisuals — Backup Configs
# =============================
# Backs up LXQt, labwc, and related configs to a timestamped tarball.
#
# Usage:
#   bash backup-configs.sh              # Interactive backup
#   bash backup-configs.sh --all        # Backup everything
#   bash backup-configs.sh --select     # Choose what to backup
#   bash backup-configs.sh --output /path/to/backup.tar.gz
#   bash backup-configs.sh --dry-run    # Preview only

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

BACKUP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/naravisuals/backups"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
OUTPUT=""
DRY_RUN=false
RUN_ALL=false
SELECT_MODE=false

# ---- Config Paths ----
# Each entry: "SOURCE_PATH|DESCRIPTION|GROUP"
CONFIGS=(
  # LXQt core
  "$HOME/.config/lxqt/*|LXQt config|lxqt"
  "$HOME/.config/lxqt-panel/*|LXQt panel|lxqt"
  "$HOME/.config/lxqt-session.conf|LXQt session|lxqt"

  # Labwc
  "$HOME/.config/labwc/*|Labwc config|compositor"
  "$HOME/.config/labwc/rc.xml|Labwc keybinds|compositor"
  "$HOME/.config/labwc/environment|Labwc environment|compositor"
  "$HOME/.config/labwc/autostart|Labwc autostart|compositor"
  "$HOME/.config/autostart/lxqt-panel.desktop|XDG autostart (lxqt-panel)|compositor"

  # Other compositors
  "$HOME/.config/hypr/*|Hyprland config|compositor"
  "$HOME/.config/sway/*|Sway config|compositor"
  "$HOME/.config/wayfire/*|Wayfire config|compositor"
  "$HOME/.config/niri/*|Niri config|compositor"
  "$HOME/.config/miriway-shell.config|Miriway config|compositor"

  # Themes
  "$HOME/.themes/*|GTK themes|themes"
  "$HOME/.icons/*|Icon themes|themes"
  "$HOME/.local/share/icons/*|User icons|themes"
  "$HOME/.local/share/fonts/*|User fonts|themes"
  "$HOME/.config/qt6ct/*|Qt6ct config|themes"
  "$HOME/.config/Kvantum/*|Kvantum themes|themes"

  # Desktop
  "$HOME/.config/wallust/*|Wallust config|desktop"
  "$HOME/.local/share/wallpapers/*|User wallpapers|desktop"
  "$HOME/.config/feh/*|Feh config|desktop"

  # Input
  "$HOME/.config/libinput/*|Libinput config|input"

  # Apps
  "$HOME/.config/dunst/*|Dunst notifications|apps"
  "$HOME/.config/rofi/*|Rofi config|apps"
  "$HOME/.config/neofetch/*|Neofetch config|apps"
  "$HOME/.config/conky/*|Conky config|apps"
)

# ---- Parse Args ----
for arg in "$@"; do
  case "$arg" in
    --all|-a) RUN_ALL=true ;;
    --select|-s) SELECT_MODE=true ;;
    --dry-run) DRY_RUN=true ;;
    --output|-o) shift; OUTPUT="$1" ;;
    --help|-h)
      printf "Backup LXQt/Labwc Configs\n\n"
      printf "Usage: bash backup-configs.sh [options]\n\n"
      printf "Options:\n"
      printf "  --all, -a       Backup everything\n"
      printf "  --select, -s    Choose what to backup\n"
      printf "  --output, -o    Custom output path\n"
      printf "  --dry-run       Preview only\n"
      printf "  --help, -h      Show this help\n"
      exit 0
      ;;
  esac
done

# ---- Select Menu ----
declare -A SELECTED=()

if [ "$SELECT_MODE" = true ]; then
  printf "\n${BOLD}Select configs to backup:${RST}\n"
  printf "${DIM}  Enter numbers separated by space, 'a' for all, 'q' to quit${RST}\n\n"

  prev_group=""
  i=1
  for entry in "${CONFIGS[@]}"; do
    IFS='|' read -r _ desc group <<< "$entry"
    if [ "$group" != "$prev_group" ]; then
      printf "\n  ${BOLD}${CYAN}── %s ──${RST}\n" "${group^^}"
      prev_group="$group"
    fi
    printf "  ${BOLD}%2d)${RST} %-25s ${DIM}%s${RST}\n" "$i" "$desc" "${group}"
    i=$((i + 1))
  done
  printf "\n  ${BOLD} a)${RST} ${GREEN}Backup All${RST}\n"
  printf "  ${BOLD} q)${RST} Quit\n"
  printf "\n${BOLD}Selection: ${RST}"
  read -r selection

  case "$selection" in
    q|Q) exit 0 ;;
    a|A) RUN_ALL=true ;;
    *)
      for num in $selection; do
        local_idx=$((num - 1))
        if [ "$local_idx" -ge 0 ] && [ "$local_idx" -lt ${#CONFIGS[@]} ]; then
          SELECTED[$local_idx]=true
        fi
      done
      ;;
  esac
fi

# Default: select all if not in select mode
if [ "$SELECT_MODE" = false ]; then
  RUN_ALL=true
fi

# ---- Build Backup ----
print_header "Backup Configs"

# Create output path
if [ -z "$OUTPUT" ]; then
  mkdir -p "$BACKUP_DIR"
  OUTPUT="$BACKUP_DIR/naravisuals-backup-$TIMESTAMP.tar.gz"
fi

log_step "Collecting configs"

# Collect files
TMPDIR="$(mktemp -d)"
BACKUP_LIST="$TMPDIR/files.txt"
> "$BACKUP_LIST"

count=0
for i in "${!CONFIGS[@]}"; do
  IFS='|' read -r pattern desc group <<< "${CONFIGS[$i]}"

  if [ "$RUN_ALL" != true ] && [ "${SELECTED[$i]:-false}" != true ]; then
    continue
  fi

  # Expand glob
  for f in $pattern; do
    if [ -e "$f" ]; then
      echo "$f" >> "$BACKUP_LIST"
      count=$((count + 1))
    fi
  done
done

if [ "$count" -eq 0 ]; then
  log_warn "No configs found to backup"
  rm -rf "$TMPDIR"
  exit 0
fi

log_ok "Found $count config files"

# Create tarball
if [ "$DRY_RUN" = true ]; then
  log_dim "[DRY-RUN] Would create: $OUTPUT"
  log_dim "[DRY-RUN] Files ($count):"
  head -20 "$BACKUP_LIST" | while read -r f; do
    log_dim "  $f"
  done
  [ "$count" -gt 20 ] && log_dim "  ... and $((count - 20)) more"
else
  tar -czf "$OUTPUT" -T "$BACKUP_LIST" 2>/dev/null
  if [ $? -eq 0 ]; then
    size=$(du -h "$OUTPUT" | cut -f1)
    log_ok "Backup created: $OUTPUT ($size)"
  else
    log_error "Failed to create backup"
    rm -rf "$TMPDIR"
    exit 1
  fi
fi

rm -rf "$TMPDIR"

# ---- Summary ----
print_summary "Backup" "ok" "$count files -> $OUTPUT"
printf "\n${BOLD}To restore:${RST}\n"
printf "  bash scripts/backup/restore-configs.sh --from %s\n" "$OUTPUT"
