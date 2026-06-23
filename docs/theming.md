# Theming Guide

Complete theming system for Naravisuals Dotfiles.

## Color Palette

The desktop uses a Nord-inspired palette for consistent aesthetics across all components.

| Element | Color | Usage |
|---------|-------|-------|
| Active border | `#81a1c1` | Focused window border |
| Inactive border | `#2e3440` | Unfocused window border |
| Active title bg | `#3b4252` | Focused window title bar |
| Active title fg | `#eceff4` | Focused window title text |
| Menu background | `#2e3440` | Right-click menu background |
| Menu active | `#81a1c1` | Menu item highlight |
| Panel background | `rgba(30,30,30,230)` | Semi-transparent panel |

## Theme Components

### 1. Window Decorations (Labwc)

**File:** `~/.config/labwc/themerc-override`

Openbox-compatible themes control window borders, title bars, and buttons.

```bash
# Install decoration themes
bash scripts/labwc-themes.sh --all

# Apply a specific theme
# Edit ~/.config/labwc/themerc-override:
#   name=Vent-dark
```

### 2. Qt Widgets (Kvantum)

**Engine:** Kvantum (SVG-based theme engine)

```bash
# Install Kvantum
bash scripts/features/install-kvantum.sh

# Configure
qt6ct  # Set Style to "kvantum"
Kvantum Manager  # Select theme (KvGlass, etc.)
```

**Environment variables:**
```bash
QT_QPA_PLATFORMTHEME=qt6ct
QT_STYLE_OVERRIDE=kvantum
```

### 3. GTK Widgets

**Files:** `~/.config/gtk-3.0/settings.ini`, `~/.config/gtk-4.0/settings.ini`

```ini
[Settings]
gtk-theme-name=Adwaita-dark
gtk-icon-theme-name=breeze
gtk-cursor-theme-name=breeze_cursors
gtk-font-name=Noto Sans 10
gtk-application-prefer-dark-theme=true
```

### 4. Panel (QSS)

**File:** `~/.config/lxqt/lxqt-panel.qss`

Qt Style Sheet for all panel plugins. Installed by `install-kvantum.sh`.

Styled elements: taskbar buttons, tray icons, volume, clock, desktop switcher, fancy menu, show desktop.

### 5. Icons

```bash
# Install icon packs
bash scripts/icons.sh --all

# Apply via LXQt settings
lxqt-config-appearance  # Icon Theme tab
```

### 6. Cursors

```bash
# Install cursor themes
bash scripts/cursors.sh --all

# Apply
lxqt-config-appearance  # Cursor Theme tab
```

### 7. Fonts

```bash
# Install Nerd Fonts
bash scripts/fonts.sh --all

# Apply
lxqt-config-appearance  # Font tab
```

## Dynamic Theming (Wallust)

Wallust generates color schemes from wallpapers and applies them across configs.

```bash
# Install wallust
bash scripts/features/wallust-setup.sh

# Apply to current wallpaper
~/.local/bin/wallust-apply /path/to/wallpaper.png
```

**What wallust generates:**
- Labwc themerc colors
- Dunst notification colors
- Rofi theme colors
- GTK settings

**Templates created at:** `~/.config/wallust/`

## Theme Installation Scripts

| Script | What it installs | Where |
|--------|-----------------|-------|
| themes.sh | GTK/Qt window themes | `~/.themes/` |
| icons.sh | Icon packs | `~/.icons/` or `~/.local/share/icons/` |
| cursors.sh | Cursor themes | `~/.icons/` or `~/.local/share/icons/` |
| fonts.sh | Nerd Fonts | `~/.local/share/fonts/` |
| wallpapers.sh | Wallpaper packs | `~/.local/share/wallpapers/` |
| labwc-themes.sh | Labwc decorations | `~/.themes/` |
| lxqt-themes.sh | LXQt widget themes | `~/.themes/` |

All scripts support `--all`, `--list`, `--dry-run`, and specific names.

## Applying Themes

After installing themes, apply them:

```bash
# Open LXQt appearance settings
lxqt-config-appearance

# Or set specific theme via command line
# GTK
gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita-dark'
gsettings set org.gnome.desktop.interface icon-theme 'breeze'

# Qt
qt6ct  # Select style, font, icon theme
```

## Reconfigure After Theme Changes

```bash
# Reload Labwc
labwc -r

# Restart panel
pkill -x lxqt-panel && lxqt-panel &

# Reload notifications
pkill -USR1 dunst 2>/dev/null || true
```
