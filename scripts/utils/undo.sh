#!/bin/bash
# Naravisuals — Undo
# ====================
# Rolls back the last install/theme change using the install log.
#
# Usage:
#   bash undo.sh                # Undo last operation
#   bash undo.sh --list         # List recent operations
#   bash undo.sh --last <n>     # Undo last N operations
#   bash undo.sh --from <file>  # Undo specific log file

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

LOG_FILE="${LOG_FILE:-/tmp/naravisuals-install.log}"
ACTION="undo"
NUM_UNDO=1

for arg in "$@"; do
  case "$arg" in
    --list) ACTION="list" ;;
    --last) shift; ACTION="last"; NUM_UNDO="$1" ;;
    --from) shift; LOG_FILE="$1" ;;
    --help|-h)
      printf "Undo Last Operation\n\n"
      printf "Usage: bash undo.sh [options]\n\n"
      printf "Options:\n"
      printf "  --list         List recent operations\n"
      printf "  --last <n>     Undo last N operations\n"
      printf "  --from <file>  Use specific log file\n"
      printf "  (no args)      Undo last operation\n"
      exit 0
      ;;
  esac
done

print_header "Undo"

# ---- Check Log ----
if [ ! -f "$LOG_FILE" ]; then
  log_error "No install log found: $LOG_FILE"
  log_info "Undo requires the install log. Run install scripts with logging enabled."
  exit 1
fi

# ---- List Recent Operations ----
list_operations() {
  log_step "Recent operations"

  if [ ! -f "$LOG_FILE" ]; then
    log_warn "No log file found"
    return 1
  fi

  # Extract STEP lines (each represents an operation)
  local count=0
  tac "$LOG_FILE" | while IFS= read -r line; do
    if [[ "$line" =~ STEP:\ (.+) ]]; then
      count=$((count + 1))
      printf "  ${BOLD}%d)${RST} %s\n" "$count" "${BASH_REMATCH[1]}"
      [ "$count" -ge 20 ] && break
    fi
  done
}

# ---- Parse Log for Files ----
get_installed_files() {
  local log="$1"
  local files=()

  # Extract file paths from OK lines (download/install patterns)
  while IFS= read -r line; do
    if [[ "$line" =~ (installed|downloaded|cloned|copied):\ (.+)$ ]]; then
      files+=("${BASH_REMATCH[2]}")
    fi
  done < "$log"

  echo "${files[@]}"
}

# ---- Undo Last Operation ----
undo_operation() {
  log_step "Undoing last operation"

  if [ ! -f "$LOG_FILE" ]; then
    log_error "No log file"
    return 1
  fi

  # Find last STEP
  local last_step
  last_step=$(tac "$LOG_FILE" | grep -m1 "STEP:" | sed 's/.*STEP: //')

  if [ -z "$last_step" ]; then
    log_warn "No operations found in log"
    return 1
  fi

  log_info "Last operation: $last_step"

  # Find what was installed
  local files=()
  while IFS= read -r line; do
    if [[ "$line" =~ (installed|downloaded|cloned):\ (.+)$ ]]; then
      local f="${BASH_REMATCH[2]}"
      # Check if path is a directory or file
      if [ -d "$f" ]; then
        files+=("$f")
      elif [ -f "$f" ]; then
        files+=("$f")
      fi
    fi
  done < "$LOG_FILE"

  if [ ${#files[@]} -eq 0 ]; then
    log_warn "No installable files found in log for this operation"
    return 1
  fi

  # Show what would be removed
  log_info "Files to remove:"
  for f in "${files[@]}"; do
    log_dim "  $f"
  done

  # Confirm
  confirm "Remove ${#files[@]} files/directories?" || return 0

  # Remove
  removed=0
  for f in "${files[@]}"; do
    if [ -d "$f" ]; then
      rm -rf "$f" 2>/dev/null && removed=$((removed + 1))
    elif [ -f "$f" ]; then
      rm -f "$f" 2>/dev/null && removed=$((removed + 1))
    fi
  done

  log_ok "Removed $removed items"

  # Remove corresponding log entries
  if [ "$DRY_RUN" != "true" ]; then
    # Mark as undone in log
    echo "[$(date '+%H:%M:%S')] UNDO: Reversed $last_step" >> "$LOG_FILE"
    log_ok "Marked as undone in log"
  fi
}

# ---- Undo Multiple ----
undo_multiple() {
  local count="$1"
  local undone=0

  for ((i = 0; i < count; i++)); do
    log_step "Undoing operation $((i + 1)) of $count"
    if undo_operation; then
      undone=$((undone + 1))
    else
      break
    fi
  done

  log_ok "Undone $undone operations"
}

# ---- Main ----
case "$ACTION" in
  list) list_operations ;;
  last) undo_multiple "$NUM_UNDO" ;;
  undo) undo_operation ;;
esac
