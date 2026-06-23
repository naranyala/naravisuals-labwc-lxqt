#!/bin/bash
# Setup Font Rendering (fontconfig)
# Ensures proper font rendering for Qt/GTK apps under Wayland
# This is critical for LXQt + labwc — without it, text/buttons appear empty
#
# Usage:
#   bash setup-fontconfig.sh              # Interactive
#   bash setup-fontconfig.sh --all        # Apply all settings
#   bash setup-fontconfig.sh --dry-run    # Preview only

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../lib.sh"

DRY_RUN=false
FORCE=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --force|-f) FORCE=true ;;
        --all|-a) FORCE=true ;;
        --help|-h)
            printf "Font Rendering Setup (fontconfig)\n\n"
            printf "Ensures proper font rendering for Qt/GTK under Wayland.\n"
            printf "Without this, text and buttons may appear empty.\n\n"
            printf "Usage: bash setup-fontconfig.sh [options]\n\n"
            printf "Options:\n"
            printf "  --all, -f        Apply all settings without prompts\n"
            printf "  --dry-run        Preview only\n"
            printf "  --help, -h       Show this help\n"
            exit 0
            ;;
    esac
done

print_header "Font Rendering Setup (fontconfig)"

# --- 1. Deploy fontconfig/fonts.conf ---
log_step "Deploying fontconfig user config"

FONTCONFIG_DIR="$HOME/.config/fontconfig"
FONTCONFIG_CONF="$FONTCONFIG_DIR/fonts.conf"
PROJECT_DIR="${SCRIPT_DIR}/../.."
SRC_CONF="$PROJECT_DIR/configs/dotfiles/fontconfig/fonts.conf"

if [ -f "$FONTCONFIG_CONF" ] && [ "$FORCE" != "true" ]; then
    log_ok "fontconfig/fonts.conf already exists"
else
    if [ -f "$SRC_CONF" ]; then
        mkdir -p "$FONTCONFIG_DIR"
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would copy fontconfig config to $FONTCONFIG_CONF"
        else
            cp "$SRC_CONF" "$FONTCONFIG_CONF"
            log_ok "Installed fontconfig/fonts.conf"
        fi
    else
        log_warn "Source fontconfig config not found at $SRC_CONF"
        log_info "Creating minimal fontconfig..."
        if [ "$DRY_RUN" != true ]; then
            mkdir -p "$FONTCONFIG_DIR"
            cat > "$FONTCONFIG_CONF" << 'FC'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <match target="font">
    <edit name="antialias" mode="assign"><bool>true</bool></edit>
  </match>
  <match target="font">
    <edit name="rgba" mode="assign"><const>rgb</const></edit>
  </match>
  <match target="font">
    <edit name="lcdfilter" mode="assign"><const>lcddefault</const></edit>
  </match>
  <match target="font">
    <edit name="hinting" mode="assign"><bool>true</bool></edit>
  </match>
  <match target="font">
    <edit name="hintstyle" mode="assign"><const>hintslight</const></edit>
  </match>
  <match target="font">
    <edit name="autohint" mode="assign"><bool>false</bool></edit>
  </match>
  <match target="pattern">
    <edit name="dpi" mode="assign"><double>96</double></edit>
  </match>
</fontconfig>
FC
            log_ok "Created minimal fontconfig config"
        fi
    fi
fi

# --- 2. Set font rendering environment variables ---
log_step "Setting font rendering environment variables"

ENV_FILE="$HOME/.config/labwc/environment"

if [ ! -f "$ENV_FILE" ]; then
    log_warn "Labwc environment file not found at $ENV_FILE"
    log_info "Deploy labwc dotfiles first, then re-run this script"
else
    # GDK_FONTCONFIG_HINT — tells GTK to use fontconfig hints
    if grep -q "GDK_FONTCONFIG_HINT" "$ENV_FILE"; then
        log_ok "GDK_FONTCONFIG_HINT already set"
    else
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would add GDK_FONTCONFIG_HINT=1"
        else
            echo "" >> "$ENV_FILE"
            echo "# Font rendering (Wayland)" >> "$ENV_FILE"
            echo "GDK_FONTCONFIG_HINT=1" >> "$ENV_FILE"
            log_ok "Added GDK_FONTCONFIG_HINT=1"
        fi
    fi

    # GDK_RENDERING — ensures proper GTK font rendering
    if grep -q "GDK_RENDERING" "$ENV_FILE"; then
        log_ok "GDK_RENDERING already set"
    else
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would add GDK_RENDERING=image"
        else
            grep -q "GDK_FONTCONFIG_HINT" "$ENV_FILE" && \
                sed -i '/^GDK_FONTCONFIG_HINT=/a GDK_RENDERING=image' "$ENV_FILE" || \
                echo "GDK_RENDERING=image" >> "$ENV_FILE"
            log_ok "Added GDK_RENDERING=image"
        fi
    fi

    # PANGOCAIRO_BACKEND — forces Pango to use fontconfig
    if grep -q "PANGOCAIRO_BACKEND" "$ENV_FILE"; then
        log_ok "PANGOCAIRO_BACKEND already set"
    else
        if [ "$DRY_RUN" = true ]; then
            log_dim "[DRY-RUN] Would add PANGOCAIRO_BACKEND=fontconfig"
        else
            grep -q "GDK_RENDERING" "$ENV_FILE" && \
                sed -i '/^GDK_RENDERING=/a PANGOCAIRO_BACKEND=fontconfig' "$ENV_FILE" || \
                echo "PANGOCAIRO_BACKEND=fontconfig" >> "$ENV_FILE"
            log_ok "Added PANGOCAIRO_BACKEND=fontconfig"
        fi
    fi
fi

# --- 3. Verify font availability ---
log_step "Verifying font availability"

REQUIRED_FONTS=("sans-serif" "monospace" "serif")
REQUIRED_FONT_NAMES=("Noto Sans" "DejaVu Sans Mono" "Noto Serif")

all_ok=true
for i in "${!REQUIRED_FONTS[@]}"; do
    generic="${REQUIRED_FONTS[$i]}"
    expected="${REQUIRED_FONT_NAMES[$i]}"
    resolved=$(fc-match "$generic" 2>/dev/null | head -1)
    if echo "$resolved" | grep -qi "$expected"; then
        log_ok "$generic -> $resolved"
    else
        log_warn "$generic -> $resolved (expected: $expected)"
        all_ok=false
    fi
done

if [ "$all_ok" = false ]; then
    log_info "Some fonts missing. Install with:"
    log_info "  sudo apt install -y fonts-noto fonts-dejavu"
fi

# --- 4. Rebuild font cache ---
log_step "Rebuilding font cache"

if [ "$DRY_RUN" = true ]; then
    log_dim "[DRY-RUN] Would rebuild font cache"
else
    if cmd_exists "fc-cache"; then
        fc-cache -f 2>/dev/null && log_ok "Font cache rebuilt" || log_warn "fc-cache failed"
    else
        log_warn "fc-cache not found"
    fi
fi

# --- 5. Ensure fontconfig is installed ---
log_step "Checking fontconfig packages"

if cmd_exists "fc-cache" && cmd_exists "fc-match"; then
    log_ok "fontconfig tools available"
else
    log_warn "fontconfig tools missing"
    if [ "$DRY_RUN" != true ]; then
        PM="$(detect_pm)"
        case "$PM" in
            apt)    sudo apt-get install -y fontconfig 2>/dev/null ;;
            dnf)    sudo dnf install -y fontconfig 2>/dev/null ;;
            pacman) sudo pacman -S --noconfirm fontconfig 2>/dev/null ;;
            *)      log_info "Install fontconfig manually" ;;
        esac
    fi
fi

# --- Summary ---
print_summary "Font Rendering Setup" "ok" "fontconfig configured. Restart session or run: labwc -r"
printf "\n${BOLD}What this did:${RST}\n"
printf "  1. Deployed ~/.config/fontconfig/fonts.conf (antialiasing, hinting, subpixel)\n"
printf "  2. Set GDK_FONTCONFIG_HINT=1, GDK_RENDERING=image, PANGOCAIRO_BACKEND=fontconfig\n"
printf "  3. Verified sans-serif/monospace/serif fonts resolve correctly\n"
printf "  4. Rebuilt font cache\n"
printf "\n${BOLD}Important:${RST} Log out and back in for env vars to take effect.\n"
