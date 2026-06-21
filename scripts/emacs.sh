#!/bin/bash
# LXQt Rice - Emacs Installer & Configurator
# ============================================
# Installs GNU Emacs GUI and deploys a minimal beginner-friendly config.
#
# Usage:
#   bash emacs.sh                     # Interactive
#   bash emacs.sh --install           # Install Emacs + config
#   bash emacs.sh --config-only       # Only write init.el (skip apt install)
#   bash emacs.sh --dry-run           # Preview

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

EMACS_CONFIG_DIR="${HOME}/.config/emacs"
EMACS_INIT="${EMACS_CONFIG_DIR}/init.el"

install_emacs_package() {
  log_step "Installing Emacs GUI"

  if cmd_exists "emacs"; then
    local ver
    ver="$(emacs --version | head -1)"
    log_ok "Emacs already installed: $ver"
    return 0
  fi

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would install: emacs (emacs-gtk)"
    return 0
  fi

  log_dim "  installing via package manager..."
  if pkg_install "emacs"; then
    log_ok "Emacs installed. Version: $(emacs --version | head -1)"
  else
    log_warn "Package manager install failed. Trying alternatives..."

    # Try Ubuntu-specific PPA for newer Emacs
    if cmd_exists "add-apt-repository"; then
      log_dim "  adding Emacs PPA for newer version..."
      sudo add-apt-repository -y ppa:ubuntu-elisp/ppa 2>/dev/null || true
      sudo apt-get update -qq 2>/dev/null || true
      sudo apt-get install -y emacs 2>/dev/null && {
        log_ok "Emacs installed from PPA"
        return 0
      }
    fi

    # Last resort: try emacs-gtk explicitly
    sudo apt-get install -y emacs-gtk 2>/dev/null && {
      log_ok "Emacs installed (emacs-gtk)"
      return 0
    }

    log_error "Could not install Emacs. Try: sudo apt install emacs"
    return 1
  fi
}

install_emacs_config() {
  log_step "Deploying Emacs configuration"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would write: $EMACS_INIT"
    return 0
  fi

  mkdir -p "$EMACS_CONFIG_DIR"

  # The init.el is embedded in dotfiles/emacs/init.el
  local src_init="$SCRIPT_DIR/../dotfiles/emacs/init.el"

  if [ -f "$EMACS_INIT" ] && [ "$FORCE" != "true" ]; then
    log_ok "Emacs config exists: $EMACS_INIT"
    log_dim "  use --force to overwrite"
    return 0
  fi

  if [ ! -f "$src_init" ]; then
    log_error "Source config not found: $src_init"
    log_dim "  Ensure dotfiles/emacs/init.el exists"
    return 1
  fi

  cp "$src_init" "$EMACS_INIT"
  log_ok "Emacs config written: $EMACS_INIT"

  # Create backup dirs
  mkdir -p "$HOME/.cache/emacs/backups" "$HOME/.cache/emacs/autosave"

  # Byte-compile for faster loading
  log_dim "  byte-compiling for faster startup..."
  emacs --batch --eval "(byte-compile-file \"$EMACS_INIT\")" 2>/dev/null && \
    log_ok "byte-compiled: ${EMACS_INIT}c" || \
    log_dim "  (byte-compilation skipped — config still works)"
}

write_desktop_entry() {
  local desktop="$HOME/.local/share/applications/emacs-lxqt.desktop"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would write: $desktop"
    return 0
  fi

  mkdir -p "$(dirname "$desktop")"

  cat > "$desktop" << 'DESKTOP'
[Desktop Entry]
Name=Emacs (LXQt Rice)
Comment=GNU Emacs with minimal beginner config
GenericName=Text Editor
Exec=emacs %F
Icon=emacs
Type=Application
Terminal=false
Categories=Development;TextEditor;
StartupWMClass=Emacs
DESKTOP

  log_ok "desktop entry written: $desktop"
  update-desktop-database "$HOME/.local/share/applications/" 2>/dev/null || true
}

show_emacs_help() {
  printf "\n"
  printf "${BOLD}${CYAN}Emacs Quick Start${RST}\n"
  printf "${DIM}%s${RST}\n" "───────────────────────────────────────────────────"
  printf "  ${BOLD}C-x C-f${RST}   Open file\n"
  printf "  ${BOLD}C-x C-s${RST}   Save\n"
  printf "  ${BOLD}C-x C-c${RST}   Quit\n"
  printf "  ${BOLD}C-x b${RST}     Switch buffer\n"
  printf "  ${BOLD}C-s${RST}       Search forward\n"
  printf "  ${BOLD}C-r${RST}       Search backward\n"
  printf "  ${BOLD}M-x${RST}       Run any command\n"
  printf "  ${BOLD}C-g${RST}       Cancel\n"
  printf "  ${BOLD}C-h t${RST}     Built-in tutorial\n"
  printf "\n"
  printf "  Config: ${DIM}~/.config/emacs/init.el${RST}\n"
  printf "  Theme:  ${DIM}modus-vivendi (dark)${RST}\n"
  printf "${DIM}%s${RST}\n" "───────────────────────────────────────────────────"
}

main() {
  print_header "LXQt Rice — Emacs Installer"

  local do_install=true
  local do_config=true

  for arg in "$@"; do
    case "$arg" in
      --install) do_install=true; do_config=true ;;
      --config-only) do_install=false; do_config=true ;;
      --dry-run) DRY_RUN=true ;;
      --force|-f) FORCE=true ;;
      --help|-h)
        printf "Usage: bash emacs.sh [options]\n\n"
        printf "Options:\n"
        printf "  --install       Install Emacs + config (default)\n"
        printf "  --config-only   Only write init.el, skip apt install\n"
        printf "  --dry-run       Preview without changes\n"
        printf "  --force, -f     Overwrite existing config\n"
        exit 0
        ;;
      *) log_warn "Unknown option: $arg";;
    esac
  done

  local ok=true

  if [ "$do_install" = true ]; then
    install_emacs_package || ok=false
  fi

  if [ "$do_config" = true ]; then
    install_emacs_config || ok=false
    write_desktop_entry || true
  fi

  if [ "$ok" = true ] || [ "$DRY_RUN" = "true" ]; then
    print_summary "Emacs setup" "ok"
    if cmd_exists "emacs"; then
      show_emacs_help
    fi
  else
    print_summary "Emacs setup" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
