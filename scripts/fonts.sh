#!/bin/bash
# LXQt Rice - Font Downloader
# ============================
# Downloads and installs programming fonts and icon fonts.
# Supports Nerd Fonts patched variants for terminal/IDE use.
#
# Usage:
#   bash fonts.sh                     # Interactive
#   bash fonts.sh --all               # Install all font families
#   bash fonts.sh --list              # List available fonts
#   bash fonts.sh jetbrains-mono      # Install specific font(s)
#   bash fonts.sh --dry-run --all     # Preview

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

# Font definitions: NAME|URL|STRIP|DESCRIPTION
# URLs point to Nerd Fonts releases for patched programming fonts
FONTS=(
  "jetbrains-mono|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/JetBrainsMono.tar.xz|0|JetBrains Mono Nerd Font (popular dev font)"
  "fira-code|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/FiraCode.tar.xz|0|Fira Code Nerd Font (ligatures)"
  "hack|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/Hack.tar.xz|0|Hack Nerd Font (classic monospace)"
  "iosevka|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/Iosevka.tar.xz|0|Iosevka Nerd Font (slender monospace)"
  "iosevka-term|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/IosevkaTerm.tar.xz|0|Iosevka Term Nerd Font (terminal optimized)"
  "nerd-fonts-symbols|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/NerdFontsSymbolsOnly.tar.xz|0|Nerd Font Symbols Only (icon glyphs)"
  "noto-sans|https://github.com/googlefonts/noto-fonts/archive/refs/heads/main.tar.gz|1|Noto Sans (comprehensive sans-serif)"
  "noto-sans-mono|https://github.com/googlefonts/noto-fonts/archive/refs/heads/main.tar.gz|1|Noto Sans Mono (monospaced)"
  "inter|https://github.com/rsms/inter/archive/refs/heads/master.tar.gz|1|Inter (high-quality UI font)"
  "cascadia-code|https://github.com/microsoft/cascadia-code/releases/download/v2404.23/CascadiaCode-2404.23.zip|0|Cascadia Code (Windows Terminal font, ligatures)"
  "fira-sans|https://github.com/mozilla/Fira/archive/refs/heads/master.tar.gz|1|Fira Sans (Mozilla UI font)"
  "sf-mono|https://github.com/epk/SF-Mono-Nerd-Font/archive/refs/heads/main.tar.gz|1|SF Mono Nerd Font (Apple's terminal font)"
  "maple-mono|https://github.com/SpaceTimee/Fusion-Maple-Mono/archive/refs/heads/main.tar.gz|1|Maple Mono Nerd Font (Chinese-friendly)"
  "victor-mono|https://github.com/ryanoasis/nerd-fonts/releases/download/v3.3.0/VictorMono.tar.xz|0|Victor Mono (cursive italic for code)"
)

list_fonts() {
  print_header "Available Fonts"
  printf "${DIM}%s${RST}\n" "Install to ~/.local/share/fonts/"
  printf "\n"
  printf "${BOLD}%-20s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "───────────────────────────────────────────────────────────────"
  for entry in "${FONTS[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-18s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install_font_pkg() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local tmpdir="$CACHE_DIR/fonts/$name"
  local target="$HOME/.local/share/fonts/$name"

  log_step "Installing font: $name"

  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_ok "font already installed: $name"
    return 0
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  download_extract "$url" "$tmpdir" "$strip" "$desc" || return 1

  if [ "$DRY_RUN" = "true" ]; then
    rm -rf "$tmpdir"
    return 0
  fi

  # Find all font files
  local font_files
  font_files="$(find "$tmpdir" -type f \( -name "*.ttf" -o -name "*.otf" \) 2>/dev/null)"

  if [ -z "$font_files" ]; then
    log_warn "No font files found in extracted archive"
    rm -rf "$tmpdir"
    return 1
  fi

  mkdir -p "$target"
  echo "$font_files" | while read -r f; do
    cp "$f" "$target/"
  done

  local count
  count="$(ls "$target"/*.{ttf,otf} 2>/dev/null | wc -l)"
  log_ok "installed $count font files: $name"

  rm -rf "$tmpdir"
}

main() {
  print_header "LXQt Rice — Font Downloader"

  require_cmds curl tar unzip find || exit 1

  local targets=()

  if [ $# -eq 0 ]; then
    list_fonts
    printf "${BOLD}Enter font names to install (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${FONTS[@]}"; do
        IFS='|' read -r name _ _ _ <<< "$entry"
        targets+=("$name")
      done
    else
      targets=($input)
    fi
  else
    for arg in "$@"; do
      case "$arg" in
        --list|-l) list_fonts; return 0 ;;
        --all|-a)
          for entry in "${FONTS[@]}"; do
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
    log_warn "No fonts selected."
    return 0
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${FONTS[@]}"; do
      IFS='|' read -r name url strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install_font_pkg "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown font: $target"
      ok=false
    fi
  done

  if [ "$ok" = true ]; then
    # Update font cache once at the end
    if cmd_exists "fc-cache"; then
      log_dim "updating font cache..."
      fc-cache -f "$HOME/.local/share/fonts" 2>/dev/null || true
    fi
    print_summary "Font downloader" "ok" "Font cache updated"
  else
    print_summary "Font downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
