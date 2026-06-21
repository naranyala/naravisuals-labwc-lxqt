#!/bin/bash
# LXQt Rice - Wallpaper Downloader
# ==================================
# Downloads wallpaper collections and sets the active wallpaper.
#
# Usage:
#   bash wallpapers.sh                    # Interactive
#   bash wallpapers.sh --all              # Download all collections
#   bash wallpapers.sh --list             # List available collections
#   bash wallpapers.sh nord-wallpapers    # Download specific collection(s)
#   bash wallpapers.sh --set nordic-aurora # Download + set active wallpaper

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

WALLPAPER_DIR="${WALLPAPER_DIR:-$HOME/Pictures/Wallpapers}"
mkdir -p "$WALLPAPER_DIR"

WALLPAPERS=(
  "nordic-wallpapers|https://github.com/linuxdotexe/nordic-wallpapers/archive/refs/heads/master.tar.gz|1|Nordic wallpaper collection (40+, 1.8k stars)"
  "nordic-wallpapers|https://github.com/nordic-theme/nordic-wallpapers/archive/refs/heads/main.tar.gz|1|Nordic wallpaper pack (official)"
  "catppuccin-wallpapers|https://github.com/catppuccin/wallpapers/archive/refs/heads/main.tar.gz|1|Catppuccin wallpaper collection (150+)"
  "gruvbox-wallpapers|https://github.com/AngelJumbo/gruvbox-wallpapers/archive/refs/heads/main.tar.gz|1|Gruvbox wallpaper collection"
  "dracula-wallpapers|https://github.com/dracula/wallpaper/archive/refs/heads/master.tar.gz|1|Dracula wallpaper collection"
  "tokyo-night-wallpapers|https://github.com/AngelJumbo/tokyo-night-wallpapers/archive/refs/heads/master.tar.gz|1|Tokyo Night wallpaper pack"
  "anime-girls-wallpapers|https://github.com/narocroc/wallpapers/archive/refs/heads/main.tar.gz|1|Anime-style wallpaper collection"
  "linux-art-wallpapers|https://github.com/sarvex/linux-art-wallpapers/archive/refs/heads/main.tar.gz|1|Linux art wallpapers"
  "minimal-wallpapers|https://github.com/sarvex/minimal-wallpapers/archive/refs/heads/main.tar.gz|1|Minimal/minimalist wallpapers"
  "dark-wallpapers|https://github.com/sarvex/dark-wallpapers/archive/refs/heads/main.tar.gz|1|Dark-themed wallpapers"
  "nature-wallpapers|https://github.com/sarvex/nature-wallpapers/archive/refs/heads/main.tar.gz|1|Nature/landscape wallpapers"
  "abstract-wallpapers|https://github.com/sarvex/abstract-wallpapers/archive/refs/heads/main.tar.gz|1|Abstract art wallpapers"
  "space-wallpapers|https://github.com/sarvex/space-wallpapers/archive/refs/heads/main.tar.gz|1|Space/astronomy wallpapers"
  "cyberpunk-wallpapers|https://github.com/sarvex/cyberpunk-wallpapers/archive/refs/heads/main.tar.gz|1|Cyberpunk/synthwave wallpapers"
)

list_wallpapers() {
  print_header "Available Wallpaper Collections"
  printf "${DIM}%s${RST}\n" "Download to: $WALLPAPER_DIR"
  printf "\n"
  printf "${BOLD}%-24s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────────────────────"
  for entry in "${WALLPAPERS[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-22s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install_wallpaper_collection() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local target="$WALLPAPER_DIR/$name"
  local tmpdir="$CACHE_DIR/wallpapers/$name"

  log_step "Downloading wallpapers: $name"

  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_ok "wallpaper collection exists: $name ($(find "$target" -type f -name '*.png' -o -name '*.jpg' -o -name '*.jpeg' 2>/dev/null | wc -l) images)"
    return 0
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir" "$target"

  download_extract "$url" "$tmpdir" "$strip" "$desc" || return 1

  local img_count=0
  find "$tmpdir" -type f \( -name "*.png" -o -name "*.jpg" -o -name "*.jpeg" -o -name "*.webp" -o -name "*.svg" \) 2>/dev/null | while read -r img; do
    cp "$img" "$target/" 2>/dev/null || true
  done

  img_count="$(find "$target" -type f \( -name "*.png" -o -name "*.jpg" -o -name "*.jpeg" -o -name "*.webp" \) 2>/dev/null | wc -l)"

  if [ "$img_count" -gt 0 ]; then
    log_ok "installed $img_count wallpapers: $name"
  else
    log_warn "no wallpaper images found in: $name"
  fi

  rm -rf "$tmpdir"
}

set_active_wallpaper() {
  local name="$1"
  local target="$WALLPAPER_DIR/$name"

  if [ ! -d "$target" ]; then
    log_error "Wallpaper collection not found: $name"
    log_dim "  Run 'bash wallpapers.sh $name' first"
    return 1
  fi

  local img
  img="$(find "$target" -type f \( -name "*.png" -o -name "*.jpg" \) 2>/dev/null | shuf -n 1)"

  if [ -z "$img" ]; then
    log_error "No images found in $target"
    return 1
  fi

  echo "$img" > "$HOME/.config/labwc/wallpaper"
  log_ok "active wallpaper set: $img"

  # Reload swaybg if running
  if pgrep swaybg &>/dev/null; then
    pkill swaybg 2>/dev/null || true
    swaybg -i "$img" -m fill &>/dev/null &
    log_dim "swaybg reloaded with new wallpaper"
  fi
}

main() {
  print_header "LXQt Rice — Wallpaper Downloader"

  require_cmds curl tar unzip find || exit 1

  local targets=()
  local set_wallpaper=false

  if [ $# -eq 0 ]; then
    list_wallpapers
    printf "${BOLD}Enter wallpaper collections to download (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${WALLPAPERS[@]}"; do
        IFS='|' read -r name _ _ _ <<< "$entry"
        targets+=("$name")
      done
    else
      targets=($input)
    fi
  else
    for arg in "$@"; do
      case "$arg" in
        --list|-l) list_wallpapers; return 0 ;;
        --all|-a)
          for entry in "${WALLPAPERS[@]}"; do
            IFS='|' read -r name _ _ _ <<< "$entry"
            targets+=("$name")
          done ;;
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        --set|-s) set_wallpaper=true ;;
        *)
          if [ "$set_wallpaper" = true ]; then
            set_active_wallpaper "$arg" || true
            set_wallpaper=false
          else
            targets+=("$arg")
          fi
          ;;
      esac
    done
  fi

  if [ ${#targets[@]} -eq 0 ]; then
    log_warn "No wallpaper collections selected."
    return 0
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${WALLPAPERS[@]}"; do
      IFS='|' read -r name url strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install_wallpaper_collection "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown collection: $target"
      ok=false
    fi
  done

  if [ "$ok" = true ]; then
    print_summary "Wallpaper downloader" "ok" "Wallpapers saved to: $WALLPAPER_DIR"
  else
    print_summary "Wallpaper downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
