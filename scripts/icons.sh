#!/bin/bash
# LXQt Rice - Icon Theme Downloader
# ==================================
# Downloads and installs icon themes for LXQt desktop.
#
# Usage:
#   bash icons.sh                    # Interactive
#   bash icons.sh --all              # Install all icon themes
#   bash icons.sh --list             # List available themes
#   bash icons.sh tela-circle       # Install specific theme(s)
#   bash icons.sh --dry-run --all    # Preview without installing

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

ICONS=(
  "tela-circle|https://github.com/vinceliuice/Tela-circle-icon-theme/archive/refs/heads/main.tar.gz|1|Tela Circle icon theme"
  "tela|https://github.com/vinceliuice/Tela-icon-theme/archive/refs/heads/master.tar.gz|1|Tela icon theme (modern)"
  "papirus|https://github.com/PapirusDevelopmentTeam/papirus-icon-theme/archive/refs/heads/master.tar.gz|1|Papirus icon theme (classic)"
  "papirus-dark|https://github.com/PapirusDevelopmentTeam/papirus-icon-theme/archive/refs/heads/master.tar.gz|1|Papirus Dark icon theme"
  "candy|https://github.com/EliverLara/candy-icons/archive/refs/heads/master.tar.gz|1|Candy icon theme (colorful)"
  "nordic|https://github.com/EliverLara/Nordic-icons/archive/refs/heads/master.tar.gz|1|Nordic icon theme"
  "catppuccin|https://github.com/catppuccin/papirus-folders/archive/refs/heads/main.tar.gz|1|Catppuccin icon theme"
  "dracula|https://github.com/dracula/gtk/archive/refs/heads/master.tar.gz|1|Dracula icon theme"
  "orchis|https://github.com/vinceliuice/Orchis-theme/archive/refs/heads/main.tar.gz|1|Orchis icon theme"
  "fluent|https://github.com/vinceliuice/Fluent-icon-theme/archive/refs/heads/main.tar.gz|1|Fluent icon theme"
  "vimix|https://github.com/vinceliuice/Vimix-icon-theme/archive/refs/heads/master.tar.gz|1|Vimix icon theme"
  "graphite|https://github.com/vinceliuice/Graphite-icon-theme/archive/refs/heads/main.tar.gz|1|Graphite icon theme"
  "we10x|https://github.com/yisus7u7/we10x-icon-theme/archive/refs/heads/main.tar.gz|1|Windows 10 style icon theme"
  "mcmojave|https://github.com/vinceliuice/McMojave-circle/archive/refs/heads/main.tar.gz|1|McMojave Circle icon theme"
  "qogir|https://github.com/vinceliuice/Qogir-icon-theme/archive/refs/heads/master.tar.gz|1|Qogir icon theme"
  "kora|https://github.com/bikass/kora/archive/refs/heads/master.tar.gz|1|Kora icon theme"
  "zafiro|https://github.com/zayronxio/Zafiro-icon-theme/archive/refs/heads/master.tar.gz|1|Zafiro icon theme"
  "reversal|https://github.com/yeyushengfan258/Reversal-icon-theme/archive/refs/heads/master.tar.gz|1|Reversal icon theme"
)

list_icons() {
  print_header "Available Icon Themes"
  printf "${DIM}%s${RST}\n" "Install to ~/.icons/"
  printf "\n"
  printf "${BOLD}%-20s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────────"
  for entry in "${ICONS[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-18s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install_icon_theme() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local tmpdir="$CACHE_DIR/icons/$name"
  local target="$HOME/.icons/$name"

  log_step "Installing icons: $name"

  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_ok "icons already installed: $name"
    return 0
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  download_extract "$url" "$tmpdir" "$strip" "$desc" || return 1

  local icon_dir
  icon_dir="$(find "$tmpdir" -maxdepth 2 -name "index.theme" -exec dirname {} \; 2>/dev/null | head -1)"

  if [ -z "$icon_dir" ]; then
    icon_dir="$(find "$tmpdir" -maxdepth 2 -type d -iname "*${name}*" 2>/dev/null | head -1)"
  fi
  if [ -z "$icon_dir" ]; then
    icon_dir="$tmpdir"
  fi

  install_icons "$icon_dir" "$name"
  rm -rf "$tmpdir"
}

main() {
  print_header "LXQt Rice — Icon Theme Downloader"

  require_cmds curl tar unzip find || exit 1

  local targets=()

  if [ $# -eq 0 ]; then
    list_icons
    printf "${BOLD}Enter icon names to install (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${ICONS[@]}"; do
        IFS='|' read -r name _ _ _ <<< "$entry"
        targets+=("$name")
      done
    else
      targets=($input)
    fi
  else
    for arg in "$@"; do
      case "$arg" in
        --list|-l) list_icons; return 0 ;;
        --all|-a)
          for entry in "${ICONS[@]}"; do
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
    log_warn "No icon themes selected. Use --list to see available themes."
    return 0
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${ICONS[@]}"; do
      IFS='|' read -r name url strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install_icon_theme "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown icon theme: $target"
      ok=false
    fi
  done

  if [ "$ok" = true ]; then
    print_summary "Icon theme downloader" "ok"
  else
    print_summary "Icon theme downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
