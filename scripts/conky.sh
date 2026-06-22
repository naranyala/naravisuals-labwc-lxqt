#!/bin/bash
# LXQt Rice - Conky Config Downloader
# =====================================
# Downloads and installs conky configuration files for
# desktop system monitoring.
#
# Usage:
#   bash conky.sh                          # Interactive
#   bash conky.sh --all                    # Install all configs
#   bash conky.sh --list                   # List available configs
#   bash conky.sh harmony                  # Install specific config(s)
#   bash conky.sh --set harmony            # Install + set as active

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib.sh"

CONKY_DIR="${CONKY_DIR:-$HOME/.config/conky}"
mkdir -p "$CONKY_DIR"

CONFIGS=(
  "harmony|https://github.com/addy-dclxvi/conky-themes/archive/refs/heads/master.tar.gz|1|Harmony conky — clean system stats"
  "google-now|https://github.com/addy-dclxvi/conky-themes/archive/refs/heads/master.tar.gz|1|Google Now style conky"
  "clear|https://github.com/addy-dclxvi/conky-themes/archive/refs/heads/master.tar.gz|1|Clear minimal conky"
  "eyecandy|https://github.com/addy-dclxvi/conky-themes/archive/refs/heads/master.tar.gz|1|Eye candy conky with visualizers"
  "nord|https://github.com/nordtheme/conky/archive/refs/heads/main.tar.gz|1|Nord-themed conky config"
  "catppuccin|https://github.com/catppuccin/conky/archive/refs/heads/main.tar.gz|1|Catppuccin Mocha conky"
  "dracula|https://github.com/dracula/conky/archive/refs/heads/master.tar.gz|1|Dracula-themed conky"
  "gruvbox|https://github.com/sarvex/gruvbox-conky/archive/refs/heads/main.tar.gz|1|Gruvbox conky config"
  "minimal|https://github.com/sarvex/minimal-conky/archive/refs/heads/main.tar.gz|1|Minimal system monitor conky"
  "rings|https://github.com/sarvex/rings-conky/archive/refs/heads/main.tar.gz|1|Rings-style conky gauges"
)

list() {
  print_header "Available Conky Configs"
  printf "${DIM}%s${RST}\n" "Install to: $CONKY_DIR"
  printf "\n"
  printf "${BOLD}%-20s %s${RST}\n" "NAME" "DESCRIPTION"
  printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────"
  for entry in "${CONFIGS[@]}"; do
    IFS='|' read -r name _ _ desc <<< "$entry"
    printf "  ${GREEN}%-18s${RST} %s\n" "$name" "$desc"
  done
  printf "\n"
}

install() {
  local name="$1" url="$2" strip="$3" desc="$4"
  local target="$CONKY_DIR/$name"
  local tmpdir="$CACHE_DIR/conky/$name"

  log_step "Installing conky config: $name"

  if [ -d "$target" ] && [ "$FORCE" != "true" ]; then
    log_ok "conky config exists: $name"
    return 0
  fi

  rm -rf "$tmpdir"
  mkdir -p "$tmpdir"

  download_extract "$url" "$tmpdir" "$strip" "$desc"

  # Find the actual conky config file
  local conky_file
  conky_file="$(find "$tmpdir" -maxdepth 3 \( -name "*.conkyrc" -o -name "conky.conf" -o -name "${name}.conf" \) 2>/dev/null | head -1)"

  if [ "$DRY_RUN" = "true" ]; then
    rm -rf "$tmpdir"
    return 0
  fi

  rm -rf "$target"
  mkdir -p "$target"

  if [ -n "$conky_file" ]; then
    cp "$conky_file" "$target/"

    # Also copy any Lua scripts or assets
    find "$tmpdir" -name "*.lua" -exec cp {} "$target/" \; 2>/dev/null || true
    log_ok "installed: $name"
  else
    # Copy everything
    cp -r "$tmpdir"/* "$target/" 2>/dev/null || true
    log_ok "installed (full directory): $name"
  fi

  rm -rf "$tmpdir"
}

set_active() {
  local name="$1"
  local config="$CONKY_DIR/$name"

  if [ ! -d "$config" ]; then
    log_error "Conky config not found: $name"
    log_dim "  Install it first: bash conky.sh $name"
    return 1
  fi

  # Find the main config file
  local main_conf
  main_conf="$(find "$config" -maxdepth 1 \( -name "*.conkyrc" -o -name "conky.conf" -o -name "${name}.conf" \) 2>/dev/null | head -1)"

  if [ -z "$main_conf" ]; then
    main_conf="$(find "$config" -type f 2>/dev/null | head -1)"
  fi

  if [ -z "$main_conf" ]; then
    log_error "No config file found in $config"
    return 1
  fi

  # Kill running conky, start with new config
  if pgrep conky &>/dev/null; then
    pkill conky 2>/dev/null || true
    sleep 0.5
  fi

  conky -c "$main_conf" -d 2>/dev/null || {
    log_warn "Could not start conky. Is it installed?"
    log_dim "  Install: sudo apt install conky-all or sudo dnf install conky"
    return 1
  }

  log_ok "active conky set: $name ($main_conf)"
}

main() {
  print_header "LXQt Rice — Conky Config Downloader"

  require_cmds curl tar find || exit 1

  local targets=()
  local do_set=false

  if [ $# -eq 0 ]; then
    list
    printf "${BOLD}Enter config names (space-separated) or 'all':${RST} "
    read -r input
    if [ "$input" = "all" ]; then
      for entry in "${CONFIGS[@]}"; do
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
          for entry in "${CONFIGS[@]}"; do
            IFS='|' read -r name _ _ _ <<< "$entry"
            targets+=("$name")
          done ;;
        --set|-s) do_set=true ;;
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        *)
          if [ "$do_set" = true ]; then
            install "$arg" "${CONFIGS[0]#*|}" 0 "" || true
            set_active "$arg" || true
            do_set=false
          else
            targets+=("$arg")
          fi
          ;;
      esac
    done
  fi

  local ok=true
  for target in "${targets[@]}"; do
    local found=false
    for entry in "${CONFIGS[@]}"; do
      IFS='|' read -r name url strip desc <<< "$entry"
      if [ "$name" = "$target" ]; then
        install "$name" "$url" "$strip" "$desc" || ok=false
        found=true
        break
      fi
    done
    if [ "$found" = false ]; then
      log_error "Unknown config: $target"
      ok=false
    fi
  done

  if [ "$ok" = true ] && [ ${#targets[@]} -gt 0 ]; then
    print_summary "Conky config downloader" "ok" "Configs in: $CONKY_DIR"
  else
    print_summary "Conky config downloader" "fail"
    return 1
  fi
}

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  main "$@"
fi
