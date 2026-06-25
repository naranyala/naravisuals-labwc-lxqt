#!/bin/bash
# Naravisuals — Restore Configs
# ===============================
# Restores configs from a naravisuals backup tarball.
#
# Usage:
#   bash restore-configs.sh --from /path/to/backup.tar.gz
#   bash restore-configs.sh --from /path/to/backup.tar.gz --dry-run
#   bash restore-configs.sh --from /path/to/backup.tar.gz --select
#   bash restore-configs.sh --latest

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

BACKUP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/naravisuals/backups"
FROM=""
DRY_RUN=false
SELECT_MODE=false
LATEST=false

# ---- Parse Args ----
for arg in "$@"; do
  case "$arg" in
    --from|-f) shift; FROM="$1" ;;
    --latest|-l) LATEST=true ;;
    --dry-run) DRY_RUN=true ;;
    --select|-s) SELECT_MODE=true ;;
    --help|-h)
      printf "Restore Naravisuals Configs\n\n"
      printf "Usage: bash restore-configs.sh [options]\n\n"
      printf "Options:\n"
      printf "  --from, -f PATH   Restore from specific backup\n"
      printf "  --latest, -l      Restore from most recent backup\n"
      printf "  --select, -s      Choose what to restore\n"
      printf "  --dry-run         Preview only\n"
      printf "  --help, -h        Show this help\n"
      exit 0
      ;;
  esac
done

# ---- Find Backup ----
if [ "$LATEST" = true ]; then
  FROM="$(ls -t "$BACKUP_DIR"/naravisuals-backup-*.tar.gz 2>/dev/null | head -1)"
  if [ -z "$FROM" ]; then
    log_error "No backups found in $BACKUP_DIR"
    exit 1
  fi
  log_info "Latest backup: $(basename "$FROM")"
fi

if [ -z "$FROM" ]; then
  log_error "Specify backup with --from or use --latest"
  log_info "Available backups:"
  ls -t "$BACKUP_DIR"/naravisuals-backup-*.tar.gz 2>/dev/null | while read -r f; do
    printf "  %s\n" "$(basename "$f")"
  done
  exit 1
fi

if [ ! -f "$FROM" ]; then
  log_error "Backup not found: $FROM"
  exit 1
fi

print_header "Restore Configs"

# ---- List Contents ----
log_step "Backup contents"
tar -tzf "$FROM" 2>/dev/null | head -30 | while read -r f; do
  log_dim "  $f"
done
total=$(tar -tzf "$FROM" 2>/dev/null | wc -l)
[ "$total" -gt 30 ] && log_dim "  ... ($total files total)"

# ---- Select ----
if [ "$SELECT_MODE" = true ]; then
  printf "\n${BOLD}Restore all configs? [Y/n] ${RST}"
  read -r answer
  case "$answer" in
    n|N|no|NO)
      log_info "Selective restore not yet supported. Restoring all."
      ;;
  esac
fi

# ---- Confirm ----
if [ "$DRY_RUN" = true ]; then
  log_info "[DRY-RUN] Would restore $total files from $(basename "$FROM")"
  log_dim "[DRY-RUN] Target: $HOME"
  exit 0
fi

confirm "Restore $total files to $HOME?" || exit 0

# ---- Restore ----
log_step "Restoring"

# Backup current configs first
PRE_RESTORE="$BACKUP_DIR/pre-restore-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$PRE_RESTORE"

# Extract to temp dir, then move
TMPDIR="$(mktemp -d)"
tar -xzf "$FROM" -C "$TMPDIR" 2>/dev/null

# Move each file, saving originals
restored=0
while IFS= read -r -d '' f; do
  rel="${f#$TMPDIR/}"
  dest="$HOME/$rel"

  # Save original if exists
  if [ -e "$dest" ]; then
    mkdir -p "$PRE_RESTORE/$(dirname "$rel")"
    cp -a "$dest" "$PRE_RESTORE/$rel" 2>/dev/null || true
  fi

  # Restore
  mkdir -p "$(dirname "$dest")"
  cp -a "$f" "$dest" 2>/dev/null
  restored=$((restored + 1))
done < <(find "$TMPDIR" -type f -print0)

rm -rf "$TMPDIR"

log_ok "Restored $restored files"
log_info "Pre-restore backup saved to: $PRE_RESTORE"

# ---- Summary ----
print_summary "Restore" "ok" "$restored files from $(basename "$FROM")"
printf "\n${BOLD}Next steps:${RST}\n"
printf "  1. Log out and back in, or restart the compositor\n"
printf "  2. Check settings with lxqt-config-appearance\n"
