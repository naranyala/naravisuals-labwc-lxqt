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

# ---- Logging ----
_LOG_FILE="${LOG_FILE:-/tmp/naravisuals-install.log}"

log_info()  { printf "${BLUE}::${RST} %s\n" "$*"; _log_to_file "INFO: $*"; }
log_ok()    { printf "${GREEN}✓${RST} %s\n" "$*"; _log_to_file " OK: $*"; }
log_warn()  { printf "${YELLOW}⚠${RST} %s\n" "$*"; _log_to_file "WARN: $*"; }
log_error() { printf "${RED}✗${RST} %s\n" "$*" >&2; _log_to_file "ERR:  $*"; }
log_step()  { printf "\n${BOLD}${CYAN}==>${RST}${BOLD} %s${RST}\n" "$*"; _log_to_file "STEP: $*"; }
log_dim()   { printf "${DIM}%s${RST}\n" "$*"; }
_log_to_file() { echo "[$(date '+%H:%M:%S')] $1" >> "$_LOG_FILE" 2>/dev/null || true; }

# ---- Error Handling ----
_cleanup_stack=()

# Register a cleanup function to run on exit
on_cleanup() { _cleanup_stack+=("$1"); }

# Die with error message and optional exit code
die() {
    local msg="$1"
    local code="${2:-1}"
    log_error "$msg"
    _run_cleanups
    exit "$code"
}

# Run all registered cleanup functions
_run_cleanups() {
    local i
    for (( i=${#_cleanup_stack[@]}-1; i>=0; i-- )); do
        eval "${_cleanup_stack[$i]}" 2>/dev/null || true
    done
}

# Trap errors and run cleanups
trap '_log_to_file "ERROR at line $LINENO (exit $?)"' ERR

# ---- Confirmation ----
# Ask Y/n question, return 0 for yes, 1 for no
confirm() {
    local prompt="${1:-Continue?}"
    if [ "$FORCE" = "true" ]; then return 0; fi
    printf "${BOLD}%s [Y/n] ${RST}" "$prompt"
    read -r answer
    case "$answer" in
        n|N|no|NO) return 1 ;;
        *) return 0 ;;
    esac
}

# ---- Pre-flight Checks ----
# Check if running as root
is_root() { [ "$(id -u)" -eq 0 ]; }

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
        log_info "Install with: sudo apt install -y ${missing[*]} (Debian/Ubuntu) or sudo dnf install -y ${missing[*]} (Fedora/RHEL)"
        return 1
    fi
}

# Check network connectivity (tries 3 hosts)
check_network() {
    local hosts=("github.com" "google.com" "1.1.1.1")
    for host in "${hosts[@]}"; do
        if ping -c1 -W3 "$host" &>/dev/null; then
            return 0
        fi
    done
    log_error "No network connectivity. Check your internet connection."
    log_info "Required for downloading themes, icons, fonts, and wallpapers."
    return 1
}

# Check available disk space (in MB)
check_disk() {
    local dir="${1:-.}"
    local needed="${2:-500}"
    local available
    available="$(df -m "$dir" 2>/dev/null | awk 'NR==2 {print $4}')"
    if [ -z "$available" ]; then
        log_warn "Could not determine disk space for $dir"
        return 0
    fi
    if [ "$available" -lt "$needed" ]; then
        log_error "Insufficient disk space in $dir: ${available}MB available, ${needed}MB+ required"
        log_info "Free up space or choose a different install location."
        return 1
    fi
    log_ok "Disk space OK: ${available}MB available in $dir"
    return 0
}

# Check if user has sudo access (without password prompt)
check_sudo() {
    if is_root; then
        return 0
    fi
    if sudo -n true 2>/dev/null; then
        return 0
    fi
    log_warn "Sudo access required for package installation."
    log_info "You may be prompted for your password during installation."
    return 0
}

# Full pre-flight check — call before any installation
check_prereqs() {
    local mode="${1:-full}"
    log_step "Pre-flight checks"

    # Required tools
    require_cmds bash tar gzip || die "Essential tools missing. Install: sudo apt install -y bash tar gzip"

    # Network (skip for minimal mode)
    if [ "$mode" != "minimal" ]; then
        check_network || die "Network required for full installation. Use --minimal for offline install."
    fi

    # Disk space
    local needed=500
    [ "$mode" = "full" ] && needed=2000
    check_disk "$HOME" "$needed" || die "Insufficient disk space."

    # Sudo
    check_sudo

    # Package manager
    local pm
    pm="$(detect_pm)"
    if [ "$pm" = "unknown" ]; then
        log_warn "No supported package manager found (apt/dnf/pacman/zypper)."
        log_info "Feature scripts may fail to install packages."
    else
        log_ok "Package manager: $pm"
    fi

    # Distro
    local distro
    distro="$(detect_distro)"
    log_ok "Detected distro: $distro"

    log_ok "Pre-flight checks passed"
}

# ---- Retry with Backoff ----
# retry <max_attempts> <delay_seconds> <command...>
retry() {
    local max_attempts="$1"
    local delay="$2"
    shift 2
    local attempt=1

    while [ $attempt -le $max_attempts ]; do
        if "$@"; then
            return 0
        fi
        log_warn "Attempt $attempt/$max_attempts failed: $*"
        if [ $attempt -lt $max_attempts ]; then
            log_info "Retrying in ${delay}s..."
            sleep "$delay"
            delay=$((delay * 2))
        fi
        attempt=$((attempt + 1))
    done

    log_error "All $max_attempts attempts failed: $*"
    return 1
}

# ---- Config ----
RESOURCES_DIR="${RESOURCES_DIR:-$HOME/.local/share/lxqt-rice}"
CACHE_DIR="${CACHE_DIR:-$RESOURCES_DIR/cache}"
DRY_RUN="${DRY_RUN:-false}"
FORCE="${FORCE:-false}"

mkdir -p "$CACHE_DIR" 2>/dev/null || true

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

# Detect distro family (debian, fedora, rhel, suse, arch, unknown)
detect_distro() {
  if [ -f /etc/os-release ]; then
    . /etc/os-release
    case "$ID" in
      ubuntu|debian|linuxmint|pop|elementary|zorin|kali|raspbian|devuan|deepin)
        echo "debian" ;;
      fedora)
        echo "fedora" ;;
      rhel|centos|rocky|alma|ol|scientific)
        echo "rhel" ;;
      opensuse*|sles)
        echo "suse" ;;
      arch|manjaro|endeavouros|garuda|artix)
        echo "arch" ;;
      void)
        echo "void" ;;
      alpine)
        echo "alpine" ;;
      *)
        echo "unknown" ;;
    esac
  elif cmd_exists "apt"; then
    echo "debian"
  elif cmd_exists "dnf"; then
    if [ -f /etc/redhat-release ]; then
      grep -qi "fedora" /etc/redhat-release 2>/dev/null && echo "fedora" || echo "rhel"
    else
      echo "rhel"
    fi
  else
    echo "unknown"
  fi
}

# Get distro version (e.g. "41", "9.3", "22.04")
detect_distro_version() {
  if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "${VERSION_ID:-unknown}"
  else
    echo "unknown"
  fi
}

# Check if a package is installed (cross-distro)
is_installed() {
  local pkg="$1"
  case "$(detect_pm)" in
    apt)     dpkg -s "$pkg" &>/dev/null ;;
    dnf)     rpm -q "$pkg" &>/dev/null ;;
    pacman)  pacman -Qi "$pkg" &>/dev/null ;;
    zypper)  rpm -q "$pkg" &>/dev/null ;;
    *)       false ;;
  esac
}

# Check if a package is available in repos (without installing)
pkg_available() {
  local pkg="$1"
  case "$(detect_pm)" in
    apt)     apt-cache show "$pkg" &>/dev/null ;;
    dnf)     dnf repoquery --available "$pkg" &>/dev/null ;;
    pacman)  pacman -Si "$pkg" &>/dev/null ;;
    zypper)  zypper search -x "$pkg" &>/dev/null ;;
    *)       false ;;
  esac
}

# Enable a Fedora Copr repo
enable_copr() {
  local repo="$1"
  if [ "$(detect_distro)" != "fedora" ]; then
    log_warn "Copr is Fedora-only, skipping: $repo"
    return 0
  fi
  if ! cmd_exists "dnf"; then
    log_error "dnf required for Copr"
    return 1
  fi
  if dnf copr list --enabled 2>/dev/null | grep -q "$repo"; then
    log_dim "  copr already enabled: $repo"
    return 0
  fi
  log_dim "  enabling copr: $repo"
  sudo dnf copr enable -y "$repo" 2>/dev/null || {
    log_warn "Failed to enable Copr: $repo"
    return 1
  }
  log_ok "Enabled Copr: $repo"
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

# Install package with name fallbacks for cross-distro compat
# Usage: pkg_install_fallback "primary-name" "fallback1" "fallback2" ...
pkg_install_fallback() {
  local names=("$@")
  local pm
  pm="$(detect_pm)"

  for name in "${names[@]}"; do
    if pkg_available "$name"; then
      log_dim "  installing: $name"
      if pkg_install "$name"; then
        log_ok "Installed: $name"
        return 0
      fi
    fi
  done

  log_error "None of these packages found: ${names[*]}"
  log_info "  Check your distro's repos or install manually."
  return 1
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
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────"
  for item in "$@"; do
    printf "  ${GREEN}•${RST} %s\n" "$item"
  done
  printf "\n"
}
