#!/bin/bash
# LXQt Rice — Global Theme Applier
# ===================================
# Reads a theme profile from configs/themes/ and applies all components atomically.
#
# Usage:
#   bash apply-theme.sh nord           # Apply a theme
#   bash apply-theme.sh --list         # List available themes
#   bash apply-theme.sh --current      # Show current theme
#   bash apply-theme.sh --preview nord # Show what would change
#
# Theme profiles live in: configs/themes/<name>.conf

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
source "$SCRIPT_DIR/scripts/utils/lib.sh"

THEMES_DIR="$SCRIPT_DIR/configs/themes"
THEMERC_DIR="$SCRIPT_DIR/configs/dotfiles/labwc"
DOTFILES_DIR="$SCRIPT_DIR/configs/dotfiles"

# ---- INI Parser ----
# Simple key=value parser that reads [section] blocks
declare -A THEME_DATA

parse_theme() {
    local file="$1"
    THEME_DATA=()
    local section=""

    while IFS= read -r line || [ -n "$line" ]; do
        line="${line%%$'\r'}"
        [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue

        if [[ "$line" =~ ^\[([a-zA-Z0-9_-]+)\]$ ]]; then
            section="${BASH_REMATCH[1]}"
            continue
        fi

        if [[ "$line" =~ ^([a-zA-Z0-9_./-]+)=(.*)$ ]]; then
            local key="${BASH_REMATCH[1]}"
            local val="${BASH_REMATCH[2]}"
            val="${val%\"}"
            val="${val#\"}"
            THEME_DATA["${section}.${key}"]="$val"
        fi
    done < "$file"
}

theme_get() {
    echo "${THEME_DATA[$1]:-}"
}

# ---- List Themes ----
list_themes() {
    print_header "Available Global Themes"
    printf "${DIM}%s${RST}\n" "Apply with: bash apply-theme.sh <name>"
    printf "\n"
    printf "${BOLD}%-20s %-12s %s${RST}\n" "NAME" "ACCENT" "DESCRIPTION"
    printf "${DIM}%s${RST}\n" "──────────────────────────────────────────────────────────────"

    for f in "$THEMES_DIR"/*.conf; do
        [ -f "$f" ] || continue
        local name
        name="$(basename "$f" .conf)"
        parse_theme "$f"
        local accent desc
        accent="$(theme_get meta.accent)"
        desc="$(theme_get meta.description)"
        printf "  ${GREEN}%-18s${RST} %-12s %s\n" "$name" "$accent" "$desc"
    done
    printf "\n"
}

# ---- Show Current Theme ----
show_current() {
    print_header "Current Theme State"

    # WM theme from rc.xml
    local rc_path="$HOME/.config/labwc/rc.xml"
    if [ -f "$rc_path" ]; then
        local wm_theme
        wm_theme="$(grep -oP '<name>\K[^<]+' "$rc_path" 2>/dev/null | head -1)"
        printf "  WM Theme:      ${CYAN}%-20s${RST} %s\n" "${wm_theme:-unknown}" "(rc.xml)"
    fi

    # GTK theme
    local gtk_path="$HOME/.config/gtk-3.0/settings.ini"
    if [ -f "$gtk_path" ]; then
        local gtk_theme
        gtk_theme="$(grep -oP 'gtk-theme-name=\K.*' "$gtk_path" 2>/dev/null | head -1)"
        printf "  GTK Theme:     ${CYAN}%-20s${RST} %s\n" "${gtk_theme:-unknown}" "(gtk-3.0/settings.ini)"
    fi

    # Qt style
    local qt_path="$HOME/.config/qt6ct/qt6ct.conf"
    if [ -f "$qt_path" ]; then
        local qt_style qt_palette
        qt_style="$(grep -oP 'style=\K.*' "$qt_path" 2>/dev/null | head -1)"
        qt_palette="$(grep -oP 'color_scheme_path=\K.*' "$qt_path" 2>/dev/null | head -1)"
        printf "  Qt Style:      ${CYAN}%-20s${RST} %s\n" "${qt_style:-unknown}" "(qt6ct.conf)"
        printf "  Qt Palette:    ${CYAN}%-20s${RST} %s\n" "$(basename "${qt_palette:-none}")" "(qt6ct.conf)"
    fi

    # Icons
    local lxqt_path="$HOME/.config/lxqt/lxqt.conf"
    if [ -f "$lxqt_path" ]; then
        local icon_theme
        icon_theme="$(grep -oP 'iconTheme=\K.*' "$lxqt_path" 2>/dev/null | head -1)"
        printf "  Icon Theme:   ${CYAN}%-20s${RST} %s\n" "${icon_theme:-unknown}" "(lxqt.conf)"
    fi

    # Cursor
    if [ -f "$gtk_path" ]; then
        local cursor_theme
        cursor_theme="$(grep -oP 'gtk-cursor-theme-name=\K.*' "$gtk_path" 2>/dev/null | head -1)"
        printf "  Cursor Theme: ${CYAN}%-20s${RST} %s\n" "${cursor_theme:-unknown}" "(gtk-3.0/settings.ini)"
    fi

    # Panel opacity
    local panel_path="$HOME/.config/lxqt/panel.conf"
    if [ -f "$panel_path" ]; then
        local panel_opacity panel_bg
        panel_opacity="$(grep -oP 'opacity=\K.*' "$panel_path" 2>/dev/null | head -1)"
        panel_bg="$(grep -oP 'background-color=\K.*' "$panel_path" 2>/dev/null | head -1)"
        printf "  Panel Opacity: ${CYAN}%-20s${RST} %s\n" "${panel_opacity:-90}" "%"
        printf "  Panel BG:     ${CYAN}%-20s${RST} %s\n" "${panel_bg:-unknown}" ""
    fi

    printf "\n"
}

# ---- Preview Theme ----
preview_theme() {
    local name="$1"
    local profile="$THEMES_DIR/$name.conf"

    if [ ! -f "$profile" ]; then
        log_error "Theme not found: $name"
        log_info "Available themes:"
        list_themes
        return 1
    fi

    parse_theme "$profile"

    print_header "Preview: $(theme_get meta.name)"
    printf "  ${DIM}%s${RST}\n\n" "$(theme_get meta.description)"

    printf "${BOLD}Components to apply:${RST}\n"
    printf "  GTK Theme:      ${GREEN}%-24s${RST} %s\n" "$(theme_get gtk.theme)" "→ ~/.themes/"
    printf "  WM Theme:       ${GREEN}%-24s${RST} %s\n" "$(theme_get wm.theme)" "→ ~/.themes/"
    printf "  WM Themerc:     ${GREEN}%-24s${RST} %s\n" "$(theme_get wm.themerc)" "→ ~/.config/labwc/themerc"
    printf "  Qt Palette:     ${GREEN}%-24s${RST} %s\n" "$(theme_get qt.palette)" "→ ~/.config/qt6ct/colors/"
    printf "  Qt Style:       ${GREEN}%-24s${RST}\n" "$(theme_get qt.style)"
    printf "  Icon Theme:     ${GREEN}%-24s${RST} %s\n" "$(theme_get icons.theme)" "→ ~/.icons/"
    printf "  Cursor Theme:   ${GREEN}%-24s${RST} %s\n" "$(theme_get cursor.theme)" "→ ~/.icons/"
    printf "  Cursor Size:    ${GREEN}%-24s${RST}\n" "$(theme_get cursor.size)"
    printf "  UI Font:        ${GREEN}%-24s${RST}\n" "$(theme_get fonts.ui)"
    printf "  Mono Font:      ${GREEN}%-24s${RST}\n" "$(theme_get fonts.mono)"
    printf "  Panel Opacity:  ${GREEN}%-24s${RST}\n" "$(theme_get panel.opacity)%"
    printf "  Panel BG:       ${GREEN}%-24s${RST}\n" "$(theme_get panel.background)"
    printf "\n"
    printf "${BOLD}Color scheme:${RST}\n"
    printf "  Active BG:    ${GREEN}$(theme_get colors.active_bg)${RST}  FG: $(theme_get colors.active_fg)\n"
    printf "  Inactive BG:  ${GREEN}$(theme_get colors.inactive_bg)${RST}  FG: $(theme_get colors.inactive_fg)\n"
    printf "  Border:       ${GREEN}$(theme_get colors.border_active)${RST} (active) / $(theme_get colors.border_inactive) (inactive)\n"
    printf "  Close:        ${RED}$(theme_get colors.button_close)${RST}  Max: ${GREEN}$(theme_get colors.button_maximize)${RST}  Min: ${YELLOW}$(theme_get colors.button_iconify)${RST}\n"
    printf "\n"
}

# ---- Apply Functions ----

apply_themerc() {
    local themerc_name
    themerc_name="$(theme_get wm.themerc)"
    [ -z "$themerc_name" ] && return 0

    local src="$THEMERC_DIR/themerc-${themerc_name}"
    local dest="$HOME/.config/labwc/themerc"

    if [ ! -f "$src" ]; then
        log_warn "themerc not found: $src (skipping)"
        return 0
    fi

    mkdir -p "$(dirname "$dest")"
    if [ -f "$dest" ]; then
        cp "$dest" "${dest}.bak"
    fi
    cp "$src" "$dest"
    log_ok "labwc themerc → $dest"
}

apply_wm_theme() {
    local theme
    theme="$(theme_get wm.theme)"
    [ -z "$theme" ] && return 0

    local rc_path="$HOME/.config/labwc/rc.xml"
    if [ ! -f "$rc_path" ]; then
        log_warn "rc.xml not found, skipping WM theme"
        return 0
    fi

    if [ -f "$rc_path" ]; then
        cp "$rc_path" "${rc_path}.bak"
    fi

    sed -i "s|<name>[^<]*</name>|<name>${theme}</name>|" "$rc_path"
    log_ok "WM theme → $theme (rc.xml)"
}

apply_gtk_theme() {
    local theme
    theme="$(theme_get gtk.theme)"
    [ -z "$theme" ] && return 0

    # Apply via gsettings if available
    if cmd_exists "gsettings"; then
        gsettings set org.gnome.desktop.interface gtk-theme "$theme" 2>/dev/null || true
    fi

    # Update GTK3 settings.ini
    for ini in "$HOME/.config/gtk-3.0/settings.ini" "$HOME/.config/gtk-4.0/settings.ini"; do
        [ -f "$ini" ] || continue
        if grep -q "gtk-theme-name=" "$ini"; then
            sed -i "s|gtk-theme-name=.*|gtk-theme-name=${theme}|" "$ini"
        else
            echo "gtk-theme-name=${theme}" >> "$ini"
        fi
    done
    log_ok "GTK theme → $theme"
}

apply_qt_theme() {
    local style palette
    style="$(theme_get qt.style)"
    palette="$(theme_get qt.palette)"

    local conf_path="$HOME/.config/qt6ct/qt6ct.conf"
    mkdir -p "$(dirname "$conf_path")"

    if [ ! -f "$conf_path" ]; then
        cat > "$conf_path" <<EOF
[Appearance]
style=${style:-Fusion}
icon_theme=breeze-dark
standard_dialogs=default
custom_palette=false
EOF
    fi

    if [ -f "$conf_path" ]; then
        cp "$conf_path" "${conf_path}.bak"
    fi

    [ -n "$style" ] && sed -i "s|^style=.*|style=${style}|" "$conf_path"

    if [ -n "$palette" ]; then
        local colors_dir="$HOME/.config/qt6ct/colors"
        mkdir -p "$colors_dir"

        # Find the palette file from installed lxqt-themes
        local src=""
        for ext in conf palette ini colors; do
            local candidate="$colors_dir/${palette}.${ext}"
            if [ -f "$candidate" ]; then
                src="$candidate"
                break
            fi
        done

        if [ -n "$src" ]; then
            sed -i "s|^color_scheme_path=.*|color_scheme_path=${src}|" "$conf_path"
            log_ok "Qt palette → $palette"
        else
            log_dim "  palette file not found for: $palette (install with lxqt-themes.sh)"
        fi
    fi

    log_ok "Qt style → ${style:-Fusion}"
}

apply_icon_theme() {
    local theme
    theme="$(theme_get icons.theme)"
    [ -z "$theme" ] && return 0

    if cmd_exists "gsettings"; then
        gsettings set org.gnome.desktop.interface icon-theme "$theme" 2>/dev/null || true
    fi

    # Update lxqt.conf
    local lxqt_path="$HOME/.config/lxqt/lxqt.conf"
    if [ -f "$lxqt_path" ]; then
        if grep -q "iconTheme=" "$lxqt_path"; then
            sed -i "s|iconTheme=.*|iconTheme=${theme}|" "$lxqt_path"
        else
            echo "iconTheme=${theme}" >> "$lxqt_path"
        fi
        if grep -q "icon_theme=" "$lxqt_path"; then
            sed -i "s|icon_theme=.*|icon_theme=${theme}|" "$lxqt_path"
        fi
    fi

    # Update session.conf
    local session_path="$HOME/.config/lxqt/session.conf"
    if [ -f "$session_path" ]; then
        if grep -q "icon_theme=" "$session_path"; then
            sed -i "s|icon_theme=.*|icon_theme=${theme}|" "$session_path"
        fi
    fi

    log_ok "Icon theme → $theme"
}

apply_cursor_theme() {
    local theme size
    theme="$(theme_get cursor.theme)"
    size="$(theme_get cursor.size)"

    if [ -n "$theme" ]; then
        if cmd_exists "gsettings"; then
            gsettings set org.gnome.desktop.interface cursor-theme "$theme" 2>/dev/null || true
        fi

        # Update GTK settings
        local gtk_path="$HOME/.config/gtk-3.0/settings.ini"
        if [ -f "$gtk_path" ]; then
            if grep -q "gtk-cursor-theme-name=" "$gtk_path"; then
                sed -i "s|gtk-cursor-theme-name=.*|gtk-cursor-theme-name=${theme}|" "$gtk_path"
            else
                echo "gtk-cursor-theme-name=${theme}" >> "$gtk_path"
            fi
        fi
        log_ok "Cursor theme → $theme"
    fi

    if [ -n "$size" ]; then
        if cmd_exists "gsettings"; then
            gsettings set org.gnome.desktop.interface cursor-size "$size" 2>/dev/null || true
        fi

        # Update labwc environment
        local env_path="$HOME/.config/labwc/environment"
        mkdir -p "$(dirname "$env_path")"
        if [ -f "$env_path" ] && grep -q "XCURSOR_SIZE=" "$env_path"; then
            sed -i "s|XCURSOR_SIZE=.*|XCURSOR_SIZE=${size}|" "$env_path"
        else
            echo "XCURSOR_SIZE=${size}" >> "$env_path"
        fi
        log_ok "Cursor size → $size"
    fi
}

apply_font_settings() {
    local ui_font mono_font
    ui_font="$(theme_get fonts.ui)"
    mono_font="$(theme_get fonts.mono)"
    wm_font="$(theme_get fonts.wm)"

    [ -z "$ui_font" ] && return 0

    local lxqt_path="$HOME/.config/lxqt/lxqt.conf"
    if [ -f "$lxqt_path" ]; then
        # Parse "Font Name,size" format
        local font_name="${ui_font%%,*}"
        local font_size="${ui_font##*,}"
        sed -i "s|^font=.*|font=${font_name},${font_size},-1,5,50,0,0,0,0,0|" "$lxqt_path"

        if [ -n "$mono_font" ]; then
            local mono_name="${mono_font%%,*}"
            local mono_size="${mono_font##*,}"
            sed -i "s|^fixedFont=.*|fixedFont=${mono_name},${mono_size},-1,5,50,0,0,0,0,0|" "$lxqt_path"
        fi
        log_ok "LXQt fonts → $ui_font"
    fi

    # Update GTK fonts
    for ini in "$HOME/.config/gtk-3.0/settings.ini" "$HOME/.config/gtk-4.0/settings.ini"; do
        [ -f "$ini" ] || continue
        if grep -q "gtk-font-name=" "$ini"; then
            sed -i "s|gtk-font-name=.*|gtk-font-name=${ui_font}|" "$ini"
        else
            echo "gtk-font-name=${ui_font}" >> "$ini"
        fi
    done

    # Update labwc rc.xml font
    local rc_path="$HOME/.config/labwc/rc.xml"
    if [ -f "$rc_path" ] && [ -n "$wm_font" ]; then
        local wm_name="${wm_font%%,*}"
        local wm_size="${wm_font##*,}"
        sed -i "s|<font place=\"ActiveWindow\">[^<]*</font>|<font place=\"ActiveWindow\">${wm_name},${wm_size}</font>|" "$rc_path"
    fi

    log_ok "Font settings applied"
}

apply_panel_settings() {
    local opacity bg
    opacity="$(theme_get panel.opacity)"
    bg="$(theme_get panel.background)"

    local panel_path="$HOME/.config/lxqt/panel.conf"
    [ -f "$panel_path" ] || return 0

    if [ -f "$panel_path" ]; then
        cp "$panel_path" "${panel_path}.bak"
    fi

    if [ -n "$opacity" ]; then
        local opacity_val
        opacity_val="$(echo "scale=2; $opacity / 100" | bc 2>/dev/null || echo "0.${opacity}")"
        # Ensure it's a proper decimal
        if [[ "$opacity_val" != *.* ]]; then
            opacity_val="0.${opacity_val}"
        fi
        sed -i "s|^opacity=.*|opacity=${opacity_val}|" "$panel_path"
    fi

    if [ -n "$bg" ]; then
        sed -i "s|^background-color=.*|background-color=${bg}|" "$panel_path"
    fi

    log_ok "Panel settings applied (opacity=${opacity:-90}%)"
}

apply_colors_to_rcxml() {
    local rc_path="$HOME/.config/labwc/rc.xml"
    [ -f "$rc_path" ] || return 0

    local border_active border_inactive
    border_active="$(theme_get colors.border_active)"
    border_inactive="$(theme_get colors.border_inactive)"

    # These are handled by themerc, but we log for completeness
    [ -n "$border_active" ] && log_ok "Window border colors set via themerc"
}

# ---- Main Apply ----
apply_theme() {
    local name="$1"
    local profile="$THEMES_DIR/$name.conf"

    if [ ! -f "$profile" ]; then
        log_error "Theme not found: $name"
        list_themes
        return 1
    fi

    parse_theme "$profile"

    local theme_name
    theme_name="$(theme_get meta.name)"

    print_header "Applying Global Theme: $theme_name"
    printf "  ${DIM}%s${RST}\n\n" "$(theme_get meta.description)"

    # Create backups
    log_step "Backing up current configs"
    local backup_dir="$HOME/.config/lxqt-rice-backup/$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$backup_dir"
    for f in "$HOME/.config/labwc/rc.xml" "$HOME/.config/labwc/themerc" \
             "$HOME/.config/qt6ct/qt6ct.conf" "$HOME/.config/lxqt/lxqt.conf" \
             "$HOME/.config/lxqt/panel.conf" "$HOME/.config/gtk-3.0/settings.ini" \
             "$HOME/.config/labwc/environment"; do
        [ -f "$f" ] && cp "$f" "$backup_dir/" 2>/dev/null || true
    done
    log_ok "Backups → $backup_dir"

    # Apply each component
    log_step "Applying theme components"
    apply_themerc
    apply_wm_theme
    apply_gtk_theme
    apply_qt_theme
    apply_icon_theme
    apply_cursor_theme
    apply_font_settings
    apply_panel_settings
    apply_colors_to_rcxml

    # Save current theme name
    local state_file="$HOME/.config/lxqt-rice/current-theme"
    mkdir -p "$(dirname "$state_file")"
    echo "$name" > "$state_file"

    print_summary "Global theme: $theme_name" "ok" \
        "Restart labwc (Mod+Shift+Q) to apply window decorations. Panel and GTK changes apply immediately."

    printf "\n${BOLD}${YELLOW}Note:${RST} Some themes may need components downloaded first.\n"
    printf "  Run: ${CYAN}bash scripts/themes.sh $(theme_get gtk.theme)${RST}\n"
    printf "  Run: ${CYAN}bash scripts/labwc-themes.sh $(theme_get wm.theme)${RST}\n"
    printf "  Run: ${CYAN}bash scripts/icons.sh $(theme_get icons.theme)${RST}\n"
    printf "  Run: ${CYAN}bash scripts/cursors.sh $(theme_get cursor.theme)${RST}\n"
    printf "  Run: ${CYAN}bash scripts/lxqt-themes.sh $(theme_get qt.palette)${RST}\n"
    printf "\n"
}

# ---- Restore from Backup ----
restore_backup() {
    local backup_dir="$HOME/.config/lxqt-rice-backup"
    if [ ! -d "$backup_dir" ]; then
        log_error "No backups found"
        return 1
    fi

    local latest
    latest="$(ls -td "$backup_dir"/*/ 2>/dev/null | head -1)"
    if [ -z "$latest" ]; then
        log_error "No backup directories found"
        return 1
    fi

    print_header "Restoring from backup"
    log_step "Restoring: $latest"

    for f in "$latest"/*; do
        [ -f "$f" ] || continue
        local name
        name="$(basename "$f")"
        local dest=""
        case "$name" in
            rc.xml)       dest="$HOME/.config/labwc/$name" ;;
            themerc)      dest="$HOME/.config/labwc/$name" ;;
            qt6ct.conf)   dest="$HOME/.config/qt6ct/$name" ;;
            lxqt.conf)    dest="$HOME/.config/lxqt/$name" ;;
            panel.conf)   dest="$HOME/.config/lxqt/$name" ;;
            settings.ini) dest="$HOME/.config/gtk-3.0/$name" ;;
            environment)  dest="$HOME/.config/labwc/$name" ;;
        esac
        if [ -n "$dest" ]; then
            mkdir -p "$(dirname "$dest")"
            cp "$f" "$dest"
            log_ok "restored: $name"
        fi
    done

    print_summary "Backup restore" "ok" "Restart labwc to apply"
}

# ---- Main ----
main() {
    case "${1:-}" in
        --list|-l)
            list_themes
            ;;
        --current|-c)
            show_current
            ;;
        --preview|-p)
            [ -z "${2:-}" ] && { log_error "Usage: apply-theme.sh --preview <name>"; exit 1; }
            preview_theme "$2"
            ;;
        --restore|-r)
            restore_backup
            ;;
        --help|-h)
            print_header "LXQt Rice — Global Theme Applier"
            printf "  ${BOLD}Usage:${RST}\n"
            printf "    bash apply-theme.sh <theme>         Apply a global theme\n"
            printf "    bash apply-theme.sh --list          List available themes\n"
            printf "    bash apply-theme.sh --current       Show current theme state\n"
            printf "    bash apply-theme.sh --preview <t>   Preview theme changes\n"
            printf "    bash apply-theme.sh --restore       Restore last backup\n"
            printf "\n"
            printf "  ${BOLD}Examples:${RST}\n"
            printf "    bash apply-theme.sh nord\n"
            printf "    bash apply-theme.sh --preview dracula\n"
            printf "    bash apply-theme.sh --list\n"
            printf "\n"
            ;;
        "")
            list_themes
            printf "${BOLD}Enter a theme name to apply:${RST} "
            read -r choice
            [ -n "$choice" ] && apply_theme "$choice"
            ;;
        *)
            apply_theme "$1"
            ;;
    esac
}

main "$@"
