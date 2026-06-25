#!/bin/bash
# Naravisuals — Multi-Monitor Fix
# =================================
# Configures and persists multi-monitor layouts for Wayland compositors.
# Supports wlr-randr (wlroots), kscreen-doctor (KDE), and niri msg (niri).
#
# Usage:
#   bash fix-multi-monitor.sh --list              # List connected monitors
#   bash fix-multi-monitor.sh --auto              # Auto-configure (mirror or extend)
#   bash fix-multi-monitor.sh --layout <file>     # Apply saved layout
#   bash fix-multi-monitor.sh --save <file>       # Save current layout
#   bash fix-multi-monitor.sh --scale <output> <scale>  # Set output scale

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../utils/lib.sh"

ACTION=""
LAYOUT_FILE=""
OUTPUT_NAME=""
SCALE_VAL=""

# ---- Parse Args ----
while [ $# -gt 0 ]; do
  case "$1" in
    --list)    ACTION="list" ;;
    --auto)    ACTION="auto" ;;
    --layout)  shift; ACTION="layout"; LAYOUT_FILE="$1" ;;
    --save)    shift; ACTION="save"; LAYOUT_FILE="$1" ;;
    --scale)   shift; ACTION="scale"; OUTPUT_NAME="$1"; shift; SCALE_VAL="$1" ;;
    --help|-h)
      printf "Multi-Monitor Configuration\n\n"
      printf "Usage: bash fix-multi-monitor.sh [options]\n\n"
      printf "Options:\n"
      printf "  --list              List connected monitors\n"
      printf "  --auto              Auto-configure (extend or mirror)\n"
      printf "  --layout <file>     Apply saved layout\n"
      printf "  --save <file>       Save current layout\n"
      printf "  --scale <out> <val> Set output scale (e.g., 1.5)\n"
      printf "  --help, -h          Show this help\n"
      exit 0
      ;;
  esac
  shift
done

# ---- Detect Tool ----
detect_tool() {
  if cmd_exists "wlr-randr"; then
    echo "wlr-randr"
  elif cmd_exists "kscreen-doctor"; then
    echo "kscreen-doctor"
  elif cmd_exists "niri"; then
    echo "niri"
  else
    echo "none"
  fi
}

TOOL="$(detect_tool)"

# ---- List Monitors ----
list_monitors() {
  case "$TOOL" in
    wlr-randr)
      log_step "Connected monitors (wlr-randr)"
      wlr-randr 2>/dev/null || log_error "wlr-randr failed. Is a Wayland compositor running?"
      ;;
    kscreen-doctor)
      log_step "Connected monitors (kscreen-doctor)"
      kscreen-doctor list 2>/dev/null || log_error "kscreen-doctor failed"
      ;;
    niri)
      log_step "Connected monitors (niri)"
      niri msg --json outputs 2>/dev/null || log_error "niri msg failed. Is niri running?"
      ;;
    none)
      log_error "No monitor configuration tool found"
      log_info "Install one of: wlr-randr, kscreen-doctor, or niri"
      return 1
      ;;
  esac
}

# ---- Save Layout ----
save_layout() {
  local outfile="${1:-monitor-layout.conf}"

  case "$TOOL" in
    wlr-randr)
      log_step "Saving layout to $outfile"
      wlr-randr > "$outfile" 2>/dev/null
      log_ok "Layout saved: $outfile"
      ;;
    *)
      log_error "Layout saving only supported with wlr-randr"
      return 1
      ;;
  esac
}

# ---- Apply Layout ----
apply_layout() {
  local infile="$1"

  if [ ! -f "$infile" ]; then
    log_error "Layout file not found: $infile"
    return 1
  fi

  case "$TOOL" in
    wlr-randr)
      log_step "Applying layout from $infile"

      # Parse wlr-randr output format and reconstruct commands
      # Look for lines like: "  <output> "..." (enabled)
      # and parse mode, position, transform, scale
      local cmd="wlr-randr"
      local current_output=""

      while IFS= read -r line; do
        # Match output lines (non-indented, before properties)
        if [[ "$line" =~ ^([A-Za-z0-9-]+)\  ]]; then
          current_output="${BASH_REMATCH[1]}"
        fi

        # Match mode
        if [[ "$line" =~ ([0-9]+x[0-9]+@[0-9.]+) ]]; then
          [ -n "$current_output" ] && cmd+=" --output $current_output --mode ${BASH_REMATCH[1]}"
        fi

        # Match position
        if [[ "$line" =~ position:\ ([0-9]+),([0-9]+) ]]; then
          [ -n "$current_output" ] && cmd+=" --pos ${BASH_REMATCH[1]}x${BASH_REMATCH[2]}"
        fi

        # Match transform
        if [[ "$line" =~ transform:\ ([a-z]+) ]]; then
          [ -n "$current_output" ] && cmd+=" --transform ${BASH_REMATCH[1]}"
        fi

        # Match scale
        if [[ "$line" =~ scale:\ ([0-9.]+) ]]; then
          [ -n "$current_output" ] && cmd+=" --scale ${BASH_REMATCH[1]}"
        fi
      done < "$infile"

      log_info "Command: $cmd"
      eval "$cmd" 2>/dev/null || {
        log_warn "Some outputs may not support the requested configuration"
        log_info "Try: wlr-randr (interactive)"
      }
      ;;
    *)
      log_error "Layout apply only supported with wlr-randr"
      return 1
      ;;
  esac
}

# ---- Auto-Configure ----
auto_configure() {
  log_step "Auto-configuring monitors"

  case "$TOOL" in
    wlr-randr)
      # Get all enabled outputs
      outputs=()
      while IFS= read -r line; do
        if [[ "$line" =~ ^([A-Za-z0-9-]+)\  ]] && [[ "$line" =~ enabled ]]; then
          outputs+=("${BASH_REMATCH[1]}")
        fi
      done < <(wlr-randr 2>/dev/null)

      if [ ${#outputs[@]} -eq 0 ]; then
        log_error "No enabled outputs found"
        return 1
      fi

      if [ ${#outputs[@]} -eq 1 ]; then
        log_info "Single monitor detected: ${outputs[0]}"
        return 0
      fi

      log_info "Found ${#outputs[@]} monitors: ${outputs[*]}"

      # Extend side by side
      x_offset=0
      for out in "${outputs[@]}"; do
        # Get preferred resolution
        mode=$(wlr-randr --output "$out" 2>/dev/null | grep -oP '\d+x\d+@\d+\.\d+' | head -1)
        if [ -z "$mode" ]; then
          log_warn "Could not get mode for $out, skipping"
          continue
        fi

        width="${mode%x*}"
        log_info "Setting $out at position ${x_offset}x0 ($mode)"
        wlr-randr --output "$out" --mode "$mode" --pos "${x_offset}x0" 2>/dev/null || true
        x_offset=$((x_offset + width))
      done
      ;;
    kscreen-doctor)
      log_info "Use KDE System Settings > Display for kscreen-doctor config"
      kscreen-doctor --outputs 2>/dev/null
      ;;
    niri)
      log_info "Niri auto-outputs: check ~/.config/niri/config.kdl"
      niri msg --json outputs 2>/dev/null
      ;;
    none)
      log_error "No monitor configuration tool found"
      return 1
      ;;
  esac
}

# ---- Set Scale ----
set_scale() {
  local output="$1"
  local scale="$2"

  case "$TOOL" in
    wlr-randr)
      log_step "Setting scale $scale for $output"
      wlr-randr --output "$output" --scale "$scale" 2>/dev/null || {
        log_error "Failed to set scale. Check output name with --list"
        return 1
      }
      log_ok "Scale set to $scale for $output"
      ;;
    *)
      log_error "Scale setting only supported with wlr-randr"
      return 1
      ;;
  esac
}

# ---- Main ----
case "${ACTION:-list}" in
  list)  list_monitors ;;
  auto)  auto_configure ;;
  save)  save_layout "$LAYOUT_FILE" ;;
  layout) apply_layout "$LAYOUT_FILE" ;;
  scale) set_scale "$OUTPUT_NAME" "$SCALE_VAL" ;;
  *)     list_monitors ;;
esac
