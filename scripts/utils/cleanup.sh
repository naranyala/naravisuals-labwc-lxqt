#!/bin/bash
# Naravisuals — Cleanup
# ======================
# Removes old caches, orphaned themes, temp files, and unused packages.
#
# Usage:
#   bash cleanup.sh              # Interactive cleanup
#   bash cleanup.sh --all        # Clean everything
#   bash cleanup.sh --dry-run    # Preview only
#   bash cleanup.sh --cache      # Cache only
#   bash cleanup.sh --themes     # Orphaned themes only

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

DRY_RUN=false
RUN_ALL=false
CLEAN_CACHE=false
CLEAN_THEMES=false
CLEAN_LOGS=false
CLEAN_BUILD=false

# ---- Parse Args ----
for arg in "$@"; do
  case "$arg" in
    --all|-a)      RUN_ALL=true ;;
    --dry-run)     DRY_RUN=true ;;
    --cache|-c)    CLEAN_CACHE=true ;;
    --themes|-t)   CLEAN_THEMES=true ;;
    --logs|-l)     CLEAN_LOGS=true ;;
    --build|-b)    CLEAN_BUILD=true ;;
    --help|-h)
      printf "Naravisuals Cleanup\n\n"
      printf "Usage: bash cleanup.sh [options]\n\n"
      printf "Options:\n"
      printf "  --all, -a       Clean everything\n"
      printf "  --cache, -c     Clean download cache\n"
      printf "  --themes, -t    Remove orphaned themes\n"
      printf "  --logs, -l      Remove old log files\n"
      printf "  --build, -b     Clean build artifacts\n"
      printf "  --dry-run       Preview only\n"
      printf "  --help, -h      Show this help\n"
      exit 0
      ;;
  esac
done

if [ "$RUN_ALL" = true ]; then
  CLEAN_CACHE=true
  CLEAN_THEMES=true
  CLEAN_LOGS=true
  CLEAN_BUILD=true
fi

# Default: show interactive menu if nothing selected
if [ "$CLEAN_CACHE" = false ] && [ "$CLEAN_THEMES" = false ] && \
   [ "$CLEAN_LOGS" = false ] && [ "$CLEAN_BUILD" = false ]; then
  printf "\n${BOLD}Select what to clean:${RST}\n\n"
  printf "  ${BOLD}1)${RST} Download cache (${DIM}~/.local/share/lxqt-rice/cache${RST})\n"
  printf "  ${BOLD}2)${RST} Orphaned themes (${DIM}~/.themes, ~/.icons without matching configs${RST})\n"
  printf "  ${BOLD}3)${RST} Old logs (${DIM}/tmp/naravisuals-install.log*${RST})\n"
  printf "  ${BOLD}4)${RST} Build artifacts (${DIM}build/ directories${RST})\n"
  printf "  ${BOLD}a)${RST} All of the above\n"
  printf "  ${BOLD}q)${RST} Quit\n"
  printf "\n${BOLD}Selection: ${RST}"
  read -r selection

  case "$selection" in
    q|Q) exit 0 ;;
    a|A) CLEAN_CACHE=true; CLEAN_THEMES=true; CLEAN_LOGS=true; CLEAN_BUILD=true ;;
    1)   CLEAN_CACHE=true ;;
    2)   CLEAN_THEMES=true ;;
    3)   CLEAN_LOGS=true ;;
    4)   CLEAN_BUILD=true ;;
    *)   log_error "Invalid selection"; exit 1 ;;
  esac
fi

print_header "Cleanup"

freed=0

# ---- Clean Cache ----
if [ "$CLEAN_CACHE" = true ]; then
  log_step "Download cache"
  cache_dir="${RESOURCES_DIR:-$HOME/.local/share/lxqt-rice}/cache"

  if [ -d "$cache_dir" ]; then
    cache_size=$(du -sh "$cache_dir" 2>/dev/null | cut -f1)
    cache_files=$(find "$cache_dir" -type f 2>/dev/null | wc -l)
    log_info "Cache: $cache_size ($cache_files files)"

    if [ "$DRY_RUN" = true ]; then
      log_dim "[DRY-RUN] Would remove: $cache_dir"
    else
      rm -rf "$cache_dir"
      mkdir -p "$cache_dir"
      log_ok "Cache cleared: $cache_size freed"
    fi
    freed=$((freed + 1))
  else
    log_dim "No cache directory found"
  fi
fi

# ---- Clean Orphaned Themes ----
if [ "$CLEAN_THEMES" = true ]; then
  log_step "Orphaned themes"

  removed=0
  for dir in "$HOME/.themes" "$HOME/.icons"; do
    [ -d "$dir" ] || continue

    while IFS= read -r theme_dir; do
      theme_name=$(basename "$theme_dir")

      # Check if theme is referenced in any config
      found=false

      # Check lxqt config
      if grep -r "$theme_name" "$HOME/.config/lxqt/" 2>/dev/null | head -1 | grep -q .; then
        found=true
      fi

      # Check labwc config
      if grep -r "$theme_name" "$HOME/.config/labwc/" 2>/dev/null | head -1 | grep -q .; then
        found=true
      fi

      # Check gtk settings
      if [ -f "$HOME/.config/gtk-3.0/settings.ini" ]; then
        if grep -q "$theme_name" "$HOME/.config/gtk-3.0/settings.ini" 2>/dev/null; then
          found=true
        fi
      fi

      if [ "$found" = false ]; then
        if [ "$DRY_RUN" = true ]; then
          log_dim "[DRY-RUN] Would remove orphaned theme: $dir/$theme_name"
        else
          log_warn "Orphaned: $theme_name"
          rm -rf "$theme_dir"
          log_ok "Removed: $theme_name"
        fi
        removed=$((removed + 1))
      fi
    done < <(find "$dir" -mindepth 1 -maxdepth 1 -type d 2>/dev/null)
  done

  [ "$removed" -eq 0 ] && log_ok "No orphaned themes found"
  freed=$((freed + removed))
fi

# ---- Clean Logs ----
if [ "$CLEAN_LOGS" = true ]; then
  log_step "Old logs"

  log_files=$(find /tmp -name "naravisuals-install*" -type f 2>/dev/null | wc -l)
  if [ "$log_files" -gt 0 ]; then
    if [ "$DRY_RUN" = true ]; then
      log_dim "[DRY-RUN] Would remove $log_files log files"
    else
      rm -f /tmp/naravisuals-install* 2>/dev/null
      log_ok "Removed $log_files log files"
    fi
    freed=$((freed + 1))
  else
    log_dim "No old logs found"
  fi
fi

# ---- Clean Build Artifacts ----
if [ "$CLEAN_BUILD" = true ]; then
  log_step "Build artifacts"

  build_dirs=$(find "$SCRIPT_DIR/../.." -name "build" -type d -maxdepth 3 2>/dev/null | wc -l)
  if [ "$build_dirs" -gt 0 ]; then
    if [ "$DRY_RUN" = true ]; then
      log_dim "[DRY-RUN] Would remove $build_dirs build directories"
    else
      find "$SCRIPT_DIR/../.." -name "build" -type d -maxdepth 3 -exec rm -rf {} + 2>/dev/null || true
      log_ok "Removed $build_dirs build directories"
    fi
    freed=$((freed + 1))
  else
    log_dim "No build directories found"
  fi
fi

# ---- Summary ----
if [ "$freed" -gt 0 ]; then
  print_summary "Cleanup" "ok" "Cleaned $freed items"
else
  log_ok "Nothing to clean — system is tidy"
fi
