#!/bin/bash
# LXQt Rice Resource Library
# ==========================
# Shared utilities for all download scripts.
# Source this file: source "$(dirname "$0")/lib.sh"

set -euo pipefail

# ---- Colors ----
RST='\033[0m'
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'

log_info()  { printf "${BLUE}::${RST} %s\n" "$*"; }
log_ok()    { printf "${GREEN}✓${RST} %s\n" "$*"; }
log_warn()  { printf "${YELLOW}⚠${RST} %s\n" "$*"; }
log_error() { printf "${RED}✗${RST} %s\n" "$*"; }
log_step()  { printf "\n${BOLD}${CYAN}==>${RST}${BOLD} %s${RST}\n" "$*"; }
log_dim()   { printf "${DIM}%s${RST}\n" "$*"; }

# ---- Config ----
RESOURCES_DIR="${RESOURCES_DIR:-$HOME/.local/share/lxqt-rice}"
CACHE_DIR="${CACHE_DIR:-$RESOURCES_DIR/cache}"
DRY_RUN="${DRY_RUN:-false}"
FORCE="${FORCE:-false}"

mkdir -p "$CACHE_DIR"

# ---- Helpers ----

# Check if a command exists
cmd_exists() { command -v "$1" &>/dev/null; }

# Check required commands, exit if missing
require_cmds() {
  local missing=()
  for cmd in "$@"; do
    if ! cmd_exists "$cmd"; then
      missing+=("$cmd")
    fi
  done
  if [ ${#missing[@]} -gt 0 ]; then
    log_error "Missing required tools: ${missing[*]}"
    log_info "Install with: sudo apt install -y ${missing[*]}"
    return 1
  fi
}

# Download a file with resume support and caching
# Usage: download <url> <output_path> [description]
download() {
  local url="$1"
  local dest="$2"
  local desc="${3:-$(basename "$url")}"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would download: $url -> $dest"
    return 0
  fi

  if [ -f "$dest" ] && [ "$FORCE" != "true" ]; then
    log_dim "  cached: $desc"
    return 0
  fi

  log_dim "  downloading: $desc"
  mkdir -p "$(dirname "$dest")"

  if cmd_exists "curl"; then
    curl -fsSL "$url" -o "$dest" --retry 3 --retry-delay 2 2>/dev/null
  elif cmd_exists "wget"; then
    wget -q --show-progress "$url" -O "$dest" 2>/dev/null
  else
    log_error "Neither curl nor wget found"
    return 1
  fi

  if [ -f "$dest" ] && [ -s "$dest" ]; then
    log_ok "$desc"
    return 0
  else
    log_error "Failed to download: $desc"
    return 1
  fi
}

# Download and extract a tar.gz/zip archive
# Usage: download_extract <url> <target_dir> [strip_components] [description]
download_extract() {
  local url="$1"
  local target="$2"
  local strip="${3:-0}"
  local desc="${4:-$(basename "$url")}"

  local ext
  ext="${url##*.}"

  local tmpdir
  tmpdir="$(mktemp -d "$CACHE_DIR/extract.XXXXXX")"
  local archive="$tmpdir/archive"

  download "$url" "$archive" "$desc" || { rm -rf "$tmpdir"; return 1; }

  if [ "$DRY_RUN" = "true" ]; then
    rm -rf "$tmpdir"
    return 0
  fi

  mkdir -p "$target"

  case "$ext" in
    xz|gz|bz2|tgz)
      tar -xaf "$archive" -C "$tmpdir" --strip-components="$strip" 2>/dev/null
      mv "$tmpdir"/* "$target"/ 2>/dev/null || true
      mv "$tmpdir"/.* "$target"/ 2>/dev/null || true
      ;;
    zip)
      unzip -qo "$archive" -d "$tmpdir" 2>/dev/null
      if [ "$strip" -gt 0 ]; then
        # Find the single top-level dir and strip it
        local top
        top="$(find "$tmpdir" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | head -1)"
        if [ -n "$top" ]; then
          mv "$top"/* "$target"/ 2>/dev/null || true
          mv "$top"/.* "$target"/ 2>/dev/null || true
        else
          mv "$tmpdir"/* "$target"/ 2>/dev/null || true
        fi
      else
        mv "$tmpdir"/* "$target"/ 2>/dev/null || true
      fi
      ;;
    *)
      cp "$archive" "$target/"
      ;;
  esac

  rm -rf "$tmpdir"
  log_ok "$desc -> $target"
}

# Clone or update a git repo shallowly
# Usage: git_clone <url> <target_dir>
git_clone() {
  local url="$1"
  local target="$2"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would clone: $url -> $target"
    return 0
  fi

  if [ -d "$target/.git" ]; then
    log_dim "  updating: $(basename "$target")"
    (cd "$target" && git pull --ff-only --depth 1 2>/dev/null) || true
    log_ok "updated: $(basename "$target")"
  else
    log_dim "  cloning: $(basename "$target")"
    mkdir -p "$(dirname "$target")"
    git clone --depth 1 "$url" "$target" 2>/dev/null
    log_ok "cloned: $(basename "$target")"
  fi
}

# Install theme to ~/.themes or system dir
# Usage: install_theme <source_dir> <theme_name>
install_theme() {
  local src="$1"
  local name="$2"
  local target="${THEME_DIR:-$HOME/.themes/$name}"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would install theme: $name -> $target"
    return 0
  fi

  mkdir -p "$(dirname "$target")"
  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_dim "  theme exists: $name"
    return 0
  fi

  rm -rf "$target"
  cp -r "$src" "$target"
  log_ok "theme installed: $name"
}

# Install icon theme to ~/.icons
# Usage: install_icons <source_dir> <icon_name>
install_icons() {
  local src="$1"
  local name="$2"
  local target="${ICON_DIR:-$HOME/.icons/$name}"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would install icons: $name -> $target"
    return 0
  fi

  mkdir -p "$(dirname "$target")"
  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_dim "  icons exist: $name"
    return 0
  fi

  rm -rf "$target"
  cp -r "$src" "$target"
  log_ok "icons installed: $name"
}

# Install fonts
# Usage: install_fonts <source_dir>
install_fonts() {
  local src="$1"
  local target="${FONT_DIR:-$HOME/.local/share/fonts}"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would install fonts from: $src"
    return 0
  fi

  mkdir -p "$target"
  find "$src" -name '*.ttf' -o -name '*.otf' | while read -r f; do
    cp "$f" "$target/"
  done

  log_ok "fonts installed to $target"
  if cmd_exists "fc-cache"; then
    fc-cache -f "$target" 2>/dev/null || true
  fi
}

# Detect available package manager
detect_pm() {
  if cmd_exists "apt"; then   echo "apt"
  elif cmd_exists "dnf"; then echo "dnf"
  elif cmd_exists "pacman"; then echo "pacman"
  elif cmd_exists "zypper"; then echo "zypper"
  else echo "unknown"
  fi
}

# Install system packages if missing
pkg_install() {
  local pm
  pm="$(detect_pm)"

  case "$pm" in
    apt)
      sudo apt-get install -y "$@" 2>/dev/null
      ;;
    dnf)
      sudo dnf install -y "$@" 2>/dev/null
      ;;
    pacman)
      sudo pacman -S --noconfirm "$@" 2>/dev/null
      ;;
    zypper)
      sudo zypper install -y "$@" 2>/dev/null
      ;;
    *)
      log_warn "Unknown package manager. Install manually: $*"
      return 1
      ;;
  esac
}

# Print a summary of what this script did
print_summary() {
  local name="$1"
  local status="$2"
  local detail="${3:-}"

  printf "\n"
  if [ "$status" = "ok" ]; then
    printf "${BOLD}${GREEN}✔${RST} ${BOLD}%s complete${RST}\n" "$name"
  else
    printf "${BOLD}${RED}✘${RST} ${BOLD}%s failed${RST}\n" "$name"
  fi
  if [ -n "$detail" ]; then
    printf "  ${DIM}%s${RST}\n" "$detail"
  fi
}

# Print a header
print_header() {
  local name="$1"
  printf "\n${BOLD}${MAGENTA}╔══════════════════════════════════════════╗${RST}\n"
  printf "${BOLD}${MAGENTA}║${RST}  ${BOLD}%s${RST}\n" "$name"
  printf "${BOLD}${MAGENTA}╚══════════════════════════════════════════╝${RST}\n"
}

# Check disk space available (in MB) for a directory
check_space() {
  local dir="$1"
  local needed="${2:-500}"
  local available
  available="$(df -m "$dir" 2>/dev/null | awk 'NR==2 {print $4}')"
  if [ "$available" -lt "$needed" ]; then
    log_warn "Low disk space in $dir: ${available}MB available, ${needed}MB+ recommended"
    return 1
  fi
  return 0
}

# List available resources that this script can download
list_resources() {
  local script_name="$1"
  shift
  printf "\n${BOLD}Available resources in ${CYAN}%s${RST}:${RST}\n" "$script_name"
  printf "${DIM}%s${RST}\n" "$separator"
  for item in "$@"; do
    printf "  ${GREEN}•${RST} %s\n" "$item"
  done
  printf "\n"
}
