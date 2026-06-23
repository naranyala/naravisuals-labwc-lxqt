# Project: Naravisuals Labwc LXQT C++ GUI Enhancements

## Architecture
This project enhances 5 existing Qt6 applications under `apps/` with backend configuration logic, CLI argument exposure, and integrated `QTest` testing suites.
All applications must respect the `XDG_CONFIG_HOME` environment variable (defaulting to `~/.config`) to allow safe, sandboxed testing.

### Target Applications & Config Files
1. **wm-gui**: Targets `labwc/rc.xml` (for `<gap>` and `<theme><cornerRadius>`) and `labwc/themerc-override` (for `window.border.width`).
2. **notifications-gui**: Targets `dunst/dunstrc` (under `[global]`, keys: `timeout`, `width`, `height`, `margin`).
3. **launcher-gui**: Targets `rofi/config.rasi` (under `window { width: ...; }`, `element-icon { size: ...; }`, and alpha channel of `bg` variable for opacity).
4. **lockscreen-gui**: Targets `swaylock/config` (for `ring-color`) and `labwc/autostart` (for `swayidle` timeouts `Dim Time`, `Lock Time`, `Suspend Time`).
5. **screenshot-gui**: Targets `screenshot/config` (under custom section or simple key-value for `Save Directory`, `Delay (s)`, `Image Format`).

## Code Layout
- `apps/<app-name>/CMakeLists.txt` - Build configuration.
- `apps/<app-name>/main.cpp` - Application entry point and UI/backend implementation.
- `apps/<app-name>/tests/` - Directory for unit tests.
- `apps/<app-name>/tests/CMakeLists.txt` - Unit test CMake build configuration.
- `apps/<app-name>/tests/test_main.cpp` - `QTest` source file.

## Milestones
| # | Name | Scope | Dependencies | Status | Conversation ID |
|---|------|-------|-------------|--------|-----------------|
| 1 | E2E Testing Track | Define and implement E2E tests for the 5 applications | None | DONE | cdc240e7-dd9a-4f65-bfeb-d7ff49bfb99b |
| 2 | wm-gui | Implement backend, CLI args, QTest for wm-gui | E2E Test Infra | IN_PROGRESS | 557771f9-6b23-42c1-8123-b62dd04672bb |
| 3 | notifications-gui | Implement backend, CLI args, QTest for notifications-gui | E2E Test Infra | IN_PROGRESS | 5ce615e2-a75f-4b8e-9f03-46fc08e00a09 |
| 4 | launcher-gui | Implement backend, CLI args, QTest for launcher-gui | E2E Test Infra | IN_PROGRESS | ce3ae68e-ef4f-440b-9145-facf8643ad10 |
| 5 | lockscreen-gui | Implement backend, CLI args, QTest for lockscreen-gui | E2E Test Infra | IN_PROGRESS | 3ca5e5cf-9e84-4b53-92a1-f2a3099858c6 |
| 6 | screenshot-gui | Implement backend, CLI args, QTest for screenshot-gui | E2E Test Infra | IN_PROGRESS | 52d10b7f-e85d-45d1-b2f1-449943516178 |

## Interface Contracts
All applications must support:
- `--help` flag to output available options.
- Headless execution: if any modification argument is provided (e.g. `--set-gaps 10`), the application must perform the update in the configuration file and exit with code 0 without launching the GUI.
- `XDG_CONFIG_HOME` overrides to point to test sandbox folders.
