# Naravisuals Dotfiles — Development Roadmap

---

## Phase 1 — Core Missing Infrastructure ✅

- [x] **Clipboard** — `configs/dotfiles/cliphist/config`, `scripts/features/install-clipboard.sh`, autostart integration
- [x] **OSD** — `configs/dotfiles/wob/config`, `scripts/features/install-osd.sh`, rc.xml patched for wob feedback
- [x] **Launcher** — `configs/dotfiles/rofi/config.rasi` (Nord theme), `scripts/features/install-launcher.sh`, Alt+F2 bound to rofi
- [x] **Portal** — `configs/dotfiles/xdg-desktop-portal-wlr/config`, `scripts/features/setup-portals.sh`, env vars added
- [x] **Dotfiles installer** — 4 new entries in `cmd/dotfiles-manager/build.cpp` manifest (28 total)
- [x] **Build deps** — wl-clipboard, rofi, wob, xdg-desktop-portal-wlr, grim, slurp added to both apt and dnf
- [x] **Orchestrator** — 4 new modules in `scripts/install-all.sh` (clipboard, osd, launcher, portals)
- [x] **Panel Reset** — `configs/dotfiles/lxqt/panel-stock.conf` (stock reference), `scripts/reset-panel.sh` (CLI with backup/restore/dry-run), Control Center "Reset Panel to Stock" button
- [x] **RPM/RHEL Support** — `lib.sh` enhanced with `detect_distro()`, `is_installed()`, `pkg_available()`, `enable_copr()`, `pkg_install_fallback()`; `switch-lxqt-compositor.sh` refactored for multi-distro; `setup-fedora-repos.sh` created; build script adds Copr on Fedora

---

## Phase 2 — Polish & Expansion ✅

- [x] **Dynamic theming** — `scripts/features/install-kvantum.sh` (Kvantum + qt6ct + env vars), `scripts/features/wallust-setup.sh` (wallust install, templates for themerc/dunst/gtk/rofi, `wallust-apply` helper), `configs/dotfiles/lxqt/lxqt-panel.qss` (Nord QSS)
- [x] **Multi-compositor** — `configs/compositors/hyprland/hyprland.conf` (full config with animations, blur, keybinds), `configs/compositors/sway/config` (i3-compatible), `configs/compositors/wayfire/wayfire.ini` (plugin-based), `scripts/features/install-compositor.sh` (installer + session.conf update)
- [x] **Control Center** — 3 new pages added: CompositorPage (install Hyprland/Sway/Wayfire + switcher), ThemingPage (Kvantum + wallust + panel reset), SystemPage (portals + Fedora repos + maintenance). Total 8 pages.
- [x] **Packaging** — `PKGBUILD` for Arch AUR (depends, optdepends, build, package functions), `install.sh` one-command installer (full/minimal/select/dry-run modes)
- [x] **Dotfiles manifest** — Expanded to 33 entries (added QSS, panel-stock, 3 compositor configs)
- [x] **Orchestrator** — Expanded to 20 modules (added Kvantum, wallust, Hyprland, Sway, Wayfire)
