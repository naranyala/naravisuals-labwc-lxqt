# Keybindings Reference

Complete keyboard shortcut reference for the Labwc session.

## Window Management

| Binding | Action | Description |
|---------|--------|-------------|
| Super+Q | Close | Close active window |
| Super+M | Toggle Maximize | Maximize/restore window |
| Super+F | Toggle Fullscreen | Enter/exit fullscreen |
| Super+X | Toggle Decorations | Show/hide window borders |
| Super+D | Toggle Desktop | Show/hide all windows |

## Application Launchers

| Binding | Action | Application |
|---------|--------|-------------|
| Super+Enter | Terminal | qterminal |
| Super+Shift+Enter | Dropdown Terminal | qterminal --drop-down |
| Alt+F2 | App Launcher | rofi (or lxqt-runner) |
| Alt+Space | App Launcher | rofi (or lxqt-runner) |
| Super+E | File Manager | pcmanfm-qt |
| Ctrl+Alt+T | Terminal | lxqt-terminal |
| Ctrl+Alt+E | Editor | emacs |

## Window Movement

| Binding | Action |
|---------|--------|
| Super+Arrow | Snap to edge (left/right/up/down) |
| Ctrl+Super+Arrow | Snap to region |
| Super+Alt+Arrow | Move window to edge |
| Super+Tab | Next window |
| Super+Shift+Tab | Previous window |

## Workspaces

| Binding | Action |
|---------|--------|
| Super+1-4 | Switch to workspace 1-4 |
| Super+Shift+1-4 | Move window to workspace 1-4 |
| Super+Shift+Left/Right | Move window to adjacent workspace |

## Audio

| Binding | Action |
|---------|--------|
| XF86AudioRaiseVolume | Volume +5% |
| XF86AudioLowerVolume | Volume -5% |
| XF86AudioMute | Toggle mute |
| XF86AudioPlay | Play/pause |
| XF86AudioNext | Next track |
| XF86AudioPrev | Previous track |
| Super+V | Open audio mixer (pavucontrol) |

## Brightness

| Binding | Action |
|---------|--------|
| XF86MonBrightnessUp | Brightness +5% |
| XF86MonBrightnessDown | Brightness -5% |

## Screenshots

| Binding | Action |
|---------|--------|
| Print Screen | Copy full screen |
| Shift+Print Screen | Copy selected area |
| Ctrl+Print Screen | Save full screen to ~/Pictures/Screenshots/ |

## System

| Binding | Action |
|---------|--------|
| Super+L | Session logout (lxqt-leave) |
| Super+F12 | Lock screen (swaylock) |
| Super+Shift+R | Reconfigure compositor (labwc -r) |
| Super+Shift+E | Open appearance config |
| Super+Shift+X | Open session config |

## Mouse Bindings

| Context | Button | Action |
|---------|--------|--------|
| Root (desktop) | Left/Right/Middle Click | Show root menu |
| Client (window) | Left Click | Focus and raise |
| Client | Left Double-Click | Toggle maximize |
| Client | Left Drag | Move window |
| Client | Right Drag | Resize window |
| TitleBar | Left Drag | Move window |
| TitleBar | Left Double-Click | Shade window |
| TitleBar | Right Click | Show client menu |

## Labwc-Specific

| Setting | Value | Effect |
|---------|-------|--------|
| `<gap>8</gap>` | 8px | Gap between tiled windows |
| `<focusMode>sloppy</focusMode>` | sloppy | Focus follows mouse |
| `<cornerRadius>10</cornerRadius>` | 10px | Window corner rounding |
| `<decorate>all</decorate>` | all | All windows get decorations |
