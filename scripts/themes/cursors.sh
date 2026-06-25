#!/bin/bash
# LXQt Rice - Cursor Theme Downloader
# =====================================
# Downloads and installs cursor themes.
#
# Usage:
#   bash cursors.sh                     # Interactive
#   bash cursors.sh --all               # Install all cursor themes
#   bash cursors.sh --list              # List available themes
#   bash cursors.sh bibata              # Install specific theme(s)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

CURSORS=(
  "bibata-modern|https://github.com/ful1e5/Bibata_Cursor/releases/download/v2.0.7/Bibata-Modern.tar.xz|0|Bibata Modern cursor (classic, sharp tips)"
  "bibata-classic|https://github.com/ful1e5/Bibata_Cursor/releases/download/v2.0.7/Bibata-Classic.tar.xz|0|Bibata Classic cursor (original)"
  "bibata-oil|https://github.com/ful1e5/Bibata_Cursor/releases/download/v2.0.7/Bibata-Oil.tar.xz|0|Bibata Oil cursor (dark)"
  "bibata-ice|https://github.com/ful1e5/Bibata_Cursor/releases/download/v2.0.7/Bibata-Ice.tar.xz|0|Bibata Ice cursor (light)"
  "capitaine|https://github.com/keeferrourke/capitaine-cursors/releases/download/r4/capitaine-cursors-r4.tar.gz|0|Capitaine cursor (macOS-like)"
  "nordic|https://github.com/EliverLara/Nordic-cursors/releases/download/v1.0/Nordic-cursors.tar.xz|0|Nordic polar cursor theme"
  "catppuccin|https://github.com/catppuccin/cursors/releases/download/v0.3.0/catppuccin-mocha-dark-cursors.zip|0|Catppuccin Mocha cursor theme"
  "dracula|https://github.com/dracula/cursors/archive/refs/heads/master.tar.gz|1|Dracula dark cursor theme"
  "volantes|https://github.com/varlesh/volantes-cursors/archive/refs/heads/master.tar.gz|1|Volantes cursors (dark/light)"
  "comix|https://github.com/varlesh/comixcursors/archive/refs/heads/master.tar.gz|1|Comix cursors (comic style)"
  "suru|https://github.com/ubports/suru-icon-theme/archive/refs/heads/master.tar.gz|1|Suru cursor theme (Ubuntu Touch)"
  "oreo|https://github.com/varlesh/oreo-cursors/archive/refs/heads/main.tar.gz|1|Oreo cursor theme (minimal)"
)

list_cursors() {
  print_header "Available Cursor Themes"
  printf "${DIM}%s${RST}\n" "Install to ~/.icons/"
  printf "\n"
  printf "${BOLD}%-20s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────────"
  for entry in "${CURSORS[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-18s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install_cursor_theme() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local tmpdir="$CACHE_DIR/cursors/$name"
  local target="$HOME/.icons/$name"

  log_step "Installing cursor theme: $name"

  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_ok "cursor theme already installed: $name"
    return 0
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  download_extract "$url" "$tmpdir" "$strip" "$desc" || return 1

  local cursor_dir
  cursor_dir="$(find "$tmpdir" -maxdepth 3 -name "cursor.theme" -exec dirname {} \; 2>/dev/null | head -1)"

  if [ -z "$cursor_dir" ]; then
    cursor_dir="$(find "$tmpdir" -maxdepth 3 -type d -name "cursors" -exec dirname {} \; 2>/dev/null | head -1)"
  fi
  if [ -z "$cursor_dir" ]; then
    cursor_dir="$(find "$tmpdir" -maxdepth 2 -type d -iname "*${name}*" 2>/dev/null | head -1)"
  fi
  if [ -z "$cursor_dir" ]; then
    cursor_dir="$tmpdir"
  fi

  install_icons "$cursor_dir" "$name"

  # Update cursor cache
  if cmd_exists "gtk-update-icon-cache"; then
    gtk-update-icon-cache -f "$target" 2>/dev/null || true
  fi

  rm -rf "$tmpdir"
}

main() {
  print_header "LXQt Rice — Cursor Theme Downloader"

  require_cmds curl tar unzip find || exit 1

  local targets=()

  if [ $# -eq 0 ]; then
    list_cursors
    printf "${BOLD}Enter cursor names to install (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${CURSORS[@]}"; do
        IFS='|' read -r name _ _ _ <<< "$entry"
        targets+=("$name")
      done
    else
      targets=($input)
    fi
  else
    for arg in "$@"; do
      case "$arg" in
        --list|-l) list_cursors; return 0 ;;
        --all|-a)
          for entry in "${CURSORS[@]}"; do
            IFS='|' read -r name _ _ _ <<< "$entry"
            targets+=("$name")
          done ;;
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        *) targets+=("$arg") ;;
      esac
    done
  fi

  if [ ${#targets[@]} -eq 0 ]; then
    log_warn "No cursor themes selected."
    return 0
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${CURSORS[@]}"; do
      IFS='|' read -r name url strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install_cursor_theme "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown cursor theme: $target"
      ok=false
    fi
  done

  if [ "$ok" = true ]; then
    print_summary "Cursor theme downloader" "ok"
  else
    print_summary "Cursor theme downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
