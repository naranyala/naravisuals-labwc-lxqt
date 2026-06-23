# Original User Request

## Initial Request — 2026-06-23T08:17:11+07:00

Develop backend logic, expose CLI arguments, and add `QTest` testing suites for 5 existing Qt6 C++ applications (`wm-gui`, `notifications-gui`, `launcher-gui`, `lockscreen-gui`, `screenshot-gui`).

Working directory: /media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt
Integrity mode: development

## Requirements

### R1. Configuration Logic Implementation
Connect the existing UI elements in the 5 GUI boilerplates to actual configuration file parsing and writing logic (e.g., editing `rc.xml` or `hyprland.conf` for the WM, `dunst` configs for notifications, etc.). The team must explore the codebase and decide what config files to target.

### R2. CLI Argument Exposure
Expose the core functionalities of each tool via command-line arguments so they can be executed headlessly (e.g., `--set-gaps 10`, `--set-timeout 5`).

### R3. Testing Suites
Create a `QTest` testing suite for each of the 5 applications inside a `tests/` directory and ensure they are integrated with CMake's `ctest`.

## Acceptance Criteria

### Testing & Verification
- [ ] Running `mkdir -p build && cd build && cmake .. && make && ctest` successfully compiles and passes 100% of the tests in all 5 application directories.
- [ ] Running each tool with its respective `--help` flag outputs the available CLI arguments.
