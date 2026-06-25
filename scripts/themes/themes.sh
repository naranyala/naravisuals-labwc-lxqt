#!/bin/bash
# LXQt Rice - Theme Downloader
# =============================
# Downloads and installs GTK, Qt, and window manager themes.
#
# Usage:
#   bash themes.sh                    # Install all themes (prompts for each)
#   bash themes.sh --all              # Install all themes non-interactively
#   bash themes.sh --list             # List available themes
#   bash themes.sh nordic            # Install specific theme(s)
#   bash themes.sh --dry-run nordic  # Preview without installing
#
# Design: independent, composable — source this file for function access.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

# ---- Theme Definitions ----
# Each entry: NAME|URL|TYPE|STRIP|DESCRIPTION
# TYPE: gtk (GTK2/3/4), qt (Qt style), window (labwc/openbox), full (everything)
THEMES=(
  "nordic|https://github.com/EliverLara/Nordic/releases/download/v2.2.0/Nordic.tar.xz|gtk|0|Nordic polar GTK theme"
  "nordic-bluish|https://github.com/EliverLara/Nordic/releases/download/v2.2.0/Nordic-bluish.tar.xz|gtk|0|Nordic bluish variant GTK theme"
  "nordic-darker|https://github.com/EliverLara/Nordic/releases/download/v2.2.0/Nordic-darker.tar.xz|gtk|0|Nordic darker variant GTK theme"
  "orchis|https://github.com/vinceliuice/Orchis-theme/archive/refs/heads/main.tar.gz|gtk|1|Orchis GTK theme (Flat Modern)"
  "graphite|https://github.com/vinceliuice/Graphite-gtk-theme/archive/refs/heads/main.tar.gz|gtk|1|Graphite GTK theme"
  "dracula|https://github.com/dracula/gtk/archive/refs/heads/master.tar.gz|gtk|1|Dracula dark GTK theme"
  "catppuccin|https://github.com/catppuccin/gtk/releases/download/v1.0.3/catppuccin-mocha-mauve.zip|gtk|0|Catppuccin Mocha GTK theme"
  "tokyo-night|https://github.com/Fausto-Korpsvart/Tokyo-Night-GTK-Theme/archive/refs/heads/main.tar.gz|gtk|1|Tokyo Night GTK theme"
  "everforest|https://github.com/Fausto-Korpsvart/Everforest-GTK-Theme/archive/refs/heads/main.tar.gz|gtk|1|Everforest GTK theme"
  "gruvbox|https://github.com/TheGreatMcPain/gruvbox-material-gtk/archive/refs/heads/main.tar.gz|gtk|1|Gruvbox Material GTK theme"
  "fluent|https://github.com/vinceliuice/Fluent-gtk-theme/archive/refs/heads/main.tar.gz|gtk|1|Fluent design GTK theme"
  "layan|https://github.com/vinceliuice/Layan-gtk-theme/archive/refs/heads/main.tar.gz|gtk|1|Layan GTK theme"
  "materia|https://github.com/nana-4/materia-theme/archive/refs/heads/master.tar.gz|gtk|1|Materia flat GTK theme"
  "vimix|https://github.com/vinceliuice/Vimix-gtk-themes/archive/refs/heads/master.tar.gz|gtk|1|Vimix GTK theme"
)

# ---- Functions ----

list_themes() {
  print_header "Available GTK/Qt Themes"
  printf "${DIM}%s${RST}\n" "Use: bash themes.sh <name> to install a specific theme"
  printf "${DIM}%s${RST}\n" "Use: bash themes.sh --all to install everything"
  printf "\n"
  printf "${BOLD}%-20s %-40s %s${RST}\n" "NAME" "TYPE" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────────────────────────"
  for entry in "${THEMES[@]}"; do
    IFS='|' read -r name url type strip desc <<< "$entry"
    printf "  ${GREEN}%-18s${RST} %-20s %s\n" "$name" "[$type]" "$desc"
  done
  printf "\n"
}

install_gtk_theme() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local tmpdir="$CACHE_DIR/themes/$name"
  local target="$HOME/.themes/$name"

  log_step "Installing theme: $name"

  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_ok "theme already installed: $name"
    log_dim "  use FORCE=true to reinstall"
    return 0
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  download_extract "$url" "$tmpdir" "$strip" "$desc" || return 1

  # Find the actual theme directory (has index.theme or gtk-3.0)
  local theme_dir
  theme_dir="$(find "$tmpdir" -maxdepth 2 -name "index.theme" -exec dirname {} \; 2>/dev/null | head -1)"

  if [ -z "$theme_dir" ]; then
    # Try to find by name pattern
    theme_dir="$(find "$tmpdir" -maxdepth 2 -type d -iname "*${name}*" 2>/dev/null | head -1)"
  fi

  if [ -z "$theme_dir" ]; then
    theme_dir="$tmpdir"
  fi

  install_theme "$theme_dir" "$name"
  rm -rf "$tmpdir"
}

# ---- Main ----
main() {
  print_header "LXQt Rice — Theme Downloader"

  require_cmds curl tar unzip find || exit 1

  local targets=()

  if [ $# -eq 0 ]; then
    # Interactive mode — list and ask
    list_themes
    printf "${BOLD}Enter theme names to install (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${THEMES[@]}"; do
        IFS='|' read -r name _ _ _ _ <<< "$entry"
        targets+=("$name")
      done
    else
      targets=($input)
    fi
  else
    for arg in "$@"; do
      case "$arg" in
        --list|-l)
          list_themes
          return 0
          ;;
        --all|-a)
          for entry in "${THEMES[@]}"; do
            IFS='|' read -r name _ _ _ _ <<< "$entry"
            targets+=("$name")
          done
          ;;
        --dry-run)
          DRY_RUN=true
          ;;
        --force|-f)
          FORCE=true
          ;;
        *)
          targets+=("$arg")
          ;;
      esac
    done
  fi

  if [ ${#targets[@]} -eq 0 ]; then
    log_warn "No themes selected. Use --list to see available themes."
    return 0
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${THEMES[@]}"; do
      IFS='|' read -r name url type strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install_gtk_theme "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown theme: $target"
      log_dim "  Use --list to see available themes"
      ok=false
    fi
  done

  if [ "$ok" = true ]; then
    print_summary "Theme downloader" "ok"
  else
    print_summary "Theme downloader" "fail"
    return 1
  fi
}

# Allow sourcing or direct execution
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
