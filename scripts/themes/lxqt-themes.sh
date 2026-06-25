#!/bin/bash
# LXQt Rice - LXQt Widget Theme Downloader
# ===========================================
# Downloads LXQt widget style themes (Qt palette themes).
# These are .conf files that go into ~/.config/lxqt/ or
# Qt palette files for qt6ct/lxqt-config-appearance.
#
# Usage:
#   bash lxqt-themes.sh                  # Interactive
#   bash lxqt-themes.sh --all            # Install all themes
#   bash lxqt-themes.sh --list           # List available themes
#   bash lxqt-themes.sh nord             # Install specific theme(s)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

THEMES=(
  "nord|https://raw.githubusercontent.com/nordtheme/qt/master/src/colors/nord.palette|palette|Nord dark palette for Qt"
  "dracula|https://raw.githubusercontent.com/dracula/qt/master/Dracula.conf|conf|Dracula Qt theme config"
  "catppuccin-mocha|https://raw.githubusercontent.com/catppuccin/qt/refs/heads/main/themes/catppuccin-mocha.conf|conf|Catppuccin Mocha Qt theme"
  "catppuccin-latte|https://raw.githubusercontent.com/catppuccin/qt/refs/heads/main/themes/catppuccin-latte.conf|conf|Catppuccin Latte Qt theme"
  "gruvbox-dark|https://raw.githubusercontent.com/Baitinq/qt-gruvbox/main/gruvbox-dark.ini|ini|Gruvbox Dark Qt palette"
  "tokyo-night|https://raw.githubusercontent.com/nicehash99/tokyo-night-qt/master/tokyo-night.colors|colors|Tokyo Night Qt colorscheme"
  "everforest|https://raw.githubusercontent.com/jacobtowne/everforest-qt/main/Everforest.conf|conf|Everforest Qt theme"
  "solarized-dark|https://raw.githubusercontent.com/AlbertVilaH/Qt-Solarized-Dark/master/Solarized%20Dark.conf|conf|Solarized Dark Qt theme"
  "arc-dark|https://raw.githubusercontent.com/arc-design/arc-theme-qt/master/colorschemes/Arc-Dark.colors|colors|Arc Dark Qt colorscheme"
  "breeze-dark|https://raw.githubusercontent.com/KDE/plasma-workspace/master/desktoptheme/breeze-dark/colors|colors|Breeze Dark Qt colorscheme (KDE)"
)

list() {
  print_header "Available LXQt Widget Themes"
  printf "${DIM}%s${RST}\n" "Install Qt palette files for lxqt-config-appearance / qt6ct"
  printf "\n"
  printf "${BOLD}%-24s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────────────────────"
  for entry in "${THEMES[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-22s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install() {
  local name="$1" url="$2" type="$3" desc="$4"
  local target="$HOME/.config/qt6ct/colors"

  log_step "Installing LXQt theme: $name"

  if [ "$DRY_RUN" = "true" ]; then
    log_dim "  [DRY-RUN] Would download: $name -> $target/"
    return 0
  fi

  mkdir -p "$target"

  local dest
  case "$type" in
    palette) dest="$target/$name.palette" ;;
    conf)    dest="$target/$name.conf" ;;
    ini)     dest="$target/$name.ini" ;;
    colors)  dest="$target/$name.colors" ;;
    *)       dest="$target/$name.conf" ;;
  esac

  if [ -f "$dest" ] && [ "$FORCE" != "true" ]; then
    log_ok "already installed: $name"
    return 0
  fi

  download "$url" "$dest" "$desc"

  if [ -s "$dest" ]; then
    log_ok "installed: $name"
    log_dim "  to: $dest"
  fi
}

main() {
  print_header "LXQt Rice — LXQt Widget Theme Downloader"

  require_cmds curl || exit 1

  local targets=()

  if [ $# -eq 0 ]; then
    list
    printf "${BOLD}Enter theme names (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${THEMES[@]}"; do
        IFS='|' read -r name _ _ _ <<< "$entry"
        targets+=("$name")
      done
    else
      targets=($input)
    fi
  else
    for arg in "$@"; do
      case "$arg" in
        --list|-l) list; return 0 ;;
        --all|-a)
          for entry in "${THEMES[@]}"; do
            IFS='|' read -r name _ _ _ <<< "$entry"
            targets+=("$name")
          done ;;
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        *) targets+=("$arg") ;;
      esac
    done
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${THEMES[@]}"; do
      IFS='|' read -r name url type desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install "$name" "$url" "$type" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown theme: $target"
      ok=false
    fi
  done

  # Auto-configure qt6ct to use the first installed theme
  local first=true
  if [ "$ok" = true ] && [ ${#targets[@]} -gt 0 ] && [ "$DRY_RUN" != "true" ]; then
    log_dim "Themes installed to ~/.config/qt6ct/colors/"
    log_dim "Set active theme via: qt6ct (GUI) or edit ~/.config/qt6ct/qt6ct.conf"
  fi

  if [ "$ok" = true ]; then
    print_summary "LXQt widget theme downloader" "ok"
  else
    print_summary "LXQt widget theme downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
