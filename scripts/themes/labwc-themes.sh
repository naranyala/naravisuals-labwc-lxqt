#!/bin/bash
# LXQt Rice - labwc/Openbox Theme Downloader
# ============================================
# Downloads window decoration themes for labwc (Openbox-compatible).
# Themes go to ~/.local/share/themes/ or ~/.themes/
#
# Usage:
#   bash labwc-themes.sh                     # Interactive
#   bash labwc-themes.sh --all               # Install all themes
#   bash labwc-themes.sh --list              # List available themes
#   bash labwc-themes.sh vent-dark           # Install specific theme(s)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

THEMES=(
  "vent-dark|https://github.com/stefonarch/lxqt-labwc-session/archive/refs/heads/main.tar.gz|1|Vent Dark — full-featured Openbox/labwc theme"
  "vent|https://github.com/stefonarch/lxqt-labwc-session/archive/refs/heads/main.tar.gz|1|Vent — light variant"
  "nordic-openbox|https://github.com/EliverLara/Nordic/archive/refs/heads/master.tar.gz|1|Nordic Openbox theme (matches Nordic GTK)"
  "dracula-openbox|https://github.com/dracula/openbox/archive/refs/heads/master.tar.gz|1|Dracula theme for Openbox"
  "catppuccin-openbox|https://github.com/catppuccin/openbox/archive/refs/heads/main.tar.gz|1|Catppuccin Mocha Openbox theme"
  "adapta-openbox|https://github.com/addy-dclxvi/openbox-theme-collections/archive/refs/heads/master.tar.gz|1|Adapta Openbox theme collection"
  "arc-openbox|https://github.com/addy-dclxvi/openbox-theme-collections/archive/refs/heads/master.tar.gz|1|Arc Openbox theme"
  "numix-openbox|https://github.com/addy-dclxvi/openbox-theme-collections/archive/refs/heads/master.tar.gz|1|Numix Openbox theme"
  "flatabulous-openbox|https://github.com/addy-dclxvi/openbox-theme-collections/archive/refs/heads/master.tar.gz|1|Flatabulous Openbox theme"
  "material-openbox|https://github.com/addy-dclxvi/openbox-theme-collections/archive/refs/heads/master.tar.gz|1|Material Openbox theme"
  "tokyo-openbox|https://github.com/addy-dclxvi/openbox-theme-collections/archive/refs/heads/master.tar.gz|1|Tokyo Night Openbox theme"
  "sweet-openbox|https://github.com/EliverLara/Sweet/archive/refs/heads/master.tar.gz|1|Sweet Openbox theme"
)

list() {
  print_header "Available labwc/Openbox Themes"
  printf "${DIM}%s${RST}\n" "Install to ~/.themes/"
  printf "\n"
  printf "${BOLD}%-24s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "────────────────────────────────────────────────────────────────────"
  for entry in "${THEMES[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-22s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local tmpdir="$CACHE_DIR/labwc-themes/$name"
  local target="$HOME/.themes"

  log_step "Installing labwc theme: $name"

  mkdir -p "$target"

  # Check if openbox-3 directory exists inside
  if [ "$FORCE" != "true" ]; then
    local existing
    existing="$(find "$target" -maxdepth 2 -type d -iname "openbox-3" -path "*/$name/*" 2>/dev/null | head -1)"
    if [ -n "$existing" ]; then
      log_ok "theme already installed: $name"
      return 0
    fi
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  download_extract "$url" "$tmpdir" "$strip" "$desc" || return 1

  # Find openbox-3 directory or prpl theme directory
  local theme_dir
  theme_dir="$(find "$tmpdir" -maxdepth 3 -type d -name "openbox-3" -exec dirname {} \; 2>/dev/null | head -1)"

  if [ -z "$theme_dir" ]; then
    # Look for the prpl-* or theme name directory
    theme_dir="$(find "$tmpdir" -maxdepth 2 -type d -iname "*${name}*" 2>/dev/null | head -1)"
  fi
  if [ -z "$theme_dir" ]; then
    theme_dir="$(find "$tmpdir" -maxdepth 2 -type d -iname "*vent*" 2>/dev/null | head -1)"
  fi
  if [ -z "$theme_dir" ]; then
    theme_dir="$tmpdir"
  fi

  install_theme "$theme_dir" "$name"
  rm -rf "$tmpdir"
}

main() {
  print_header "LXQt Rice — labwc/Openbox Theme Downloader"

  require_cmds curl tar unzip find || exit 1

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
      IFS='|' read -r name url strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown theme: $target"
      ok=false
    fi
  done

  if [ "$ok" = true ]; then
    print_summary "labwc theme downloader" "ok"
  else
    print_summary "labwc theme downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
