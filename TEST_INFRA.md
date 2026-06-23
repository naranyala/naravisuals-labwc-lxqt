# Naravisuals Labwc LXQt E2E Testing Infrastructure

This document outlines the design, sandboxing layout, features mapping, and comprehensive test plan for the End-to-End (E2E) testing framework designed for the five custom system configuration utilities.

---

## 1. Test Philosophy & Sandboxing Layout

### Philosophy
E2E testing verifies that:
1. Custom Qt6 graphical configuration utilities correctly translate user actions into precise changes in target system configuration files.
2. The applications can run both in headless mode (for CLI/scripted configurations) and GUI mode (for interactive usage).
3. The configuration changes conform to syntax requirements, bounds checks, and cross-application constraints.

### Sandboxing Layout
To prevent E2E tests from corrupting or modifying the active user's home configuration files, the test runner isolates all runs inside a temporary directory.
- The test runner sets the `XDG_CONFIG_HOME` environment variable to a unique temporary directory for each test case execution.
- Prior to executing a test case, template configuration files from `configs/dotfiles/` are copied into the sandboxed directory to establish a known starting state.
- For tests examining missing or empty configuration scenarios, the sandbox directory is deliberately left empty or selectively pruned.
- The `QT_QPA_PLATFORM` environment variable is set to `offscreen` to ensure headless application execution during standard CLI parameter tests.

```
[System Template Configs]
          │ (configs/dotfiles/*)
          ▼ (copied at setUp)
   [Sandbox Directory] (XDG_CONFIG_HOME=/tmp/tmpXXXXXX)
          │
          ├─ labwc/rc.xml
          ├─ labwc/themerc-override
          ├─ dunst/dunstrc
          ├─ rofi/config.rasi
          ├─ swaylock/config
          ├─ labwc/autostart
          └─ screenshot/config
```

---

## 2. Features Mapping

| Application | Executable Name | Target Config Files | Key Customizations | CLI Flags |
|---|---|---|---|---|
| **wm-gui** | `nv-wm-gui` | `labwc/rc.xml`<br>`labwc/themerc-override` | `<gap>` (margin)<br>`<theme><cornerRadius>` (window styling)<br>`window.border.width` (border thickness) | `--set-gaps <int>`<br>`--set-corner-radius <int>`<br>`--set-border-width <int>` |
| **notifications-gui** | `nv-notifications-gui` | `dunst/dunstrc` | `timeout`<br>`width`<br>`height`<br>`margin` | `--set-timeout <int>`<br>`--set-width <int>`<br>`--set-height <int>`<br>`--set-margin <int>` |
| **launcher-gui** | `nv-launcher-gui` | `rofi/config.rasi` | `window { width: ...; }`<br>`element-icon { size: ...; }`<br>`bg` alpha (opacity) | `--set-width <int>`<br>`--set-icon-size <int>`<br>`--set-opacity <0-100>` |
| **lockscreen-gui** | `nv-lockscreen-gui` | `swaylock/config`<br>`labwc/autostart` | `ring-color` (swaylock style)<br>`swayidle` timeouts (Dim, Lock, Suspend) | `--set-ring-color <hex>`<br>`--set-dim-time <int>`<br>`--set-lock-time <int>`<br>`--set-suspend-time <int>` |
| **screenshot-gui** | `nv-screenshot-gui` | `screenshot/config` | `Save Directory`<br>`Delay (s)`<br>`Image Format` | `--set-save-dir <path>`<br>`--set-delay <int>`<br>`--set-image-format <str>` |

---

## 3. Comprehensive 4-Tier Test Case Plan

The E2E test suite comprises exactly **60 test cases** divided into 4 distinct tiers:

### Tier 1: Feature Coverage (30 Cases)
Focuses on testing standard, valid configuration requests for individual applications.

#### WmGuiTests (6 Cases)
1. **`test_wm_set_gap_valid`**
   - *Description*: Set window manager gap to a valid value (12).
   - *Assertions*: Check `labwc/rc.xml` exists, and node `<core><gap>` contains `12`.
2. **`test_wm_set_corner_radius_valid`**
   - *Description*: Set window corner radius to a valid value (15).
   - *Assertions*: Check `labwc/rc.xml` contains `<theme><cornerRadius>` set to `15`.
3. **`test_wm_set_border_width_valid`**
   - *Description*: Set window border width to a valid value (4).
   - *Assertions*: Check `labwc/themerc-override` contains `window.border.width: 4`.
4. **`test_wm_gui_launch`**
   - *Description*: Run application without parameters to start GUI.
   - *Assertions*: Verify the process blocks and runs without crashing for 2 seconds, then terminates.
5. **`test_wm_help`**
   - *Description*: Run application with `--help`.
   - *Assertions*: Verify exit code 0 and presence of keywords `gap`, `corner-radius`, `border-width`.
6. **`test_wm_multiple_valid_together`**
   - *Description*: Set all three WM settings concurrently.
   - *Assertions*: Verify `<gap>`, `<cornerRadius>`, and `window.border.width` are updated to correct values.

#### NotificationsGuiTests (6 Cases)
7. **`test_notifications_set_timeout_valid`**
   - *Description*: Set notifications timeout (15 seconds).
   - *Assertions*: Verify `dunst/dunstrc` key `timeout` under `[global]` is `15`.
8. **`test_notifications_set_width_valid`**
   - *Description*: Set notifications panel width (450).
   - *Assertions*: Verify `dunst/dunstrc` key `width` under `[global]` is `450`.
9. **`test_notifications_set_height_valid`**
   - *Description*: Set notifications panel height (150).
   - *Assertions*: Verify `dunst/dunstrc` key `height` under `[global]` is `150`.
10. **`test_notifications_set_margin_valid`**
    - *Description*: Set notifications margin (25).
    - *Assertions*: Verify `dunst/dunstrc` key `margin` under `[global]` is `25`.
11. **`test_notifications_gui_launch`**
    - *Description*: Run notifications GUI headlessly.
    - *Assertions*: Verify it runs, blocks, and can be terminated.
12. **`test_notifications_help`**
    - *Description*: Run notifications GUI with `--help`.
    - *Assertions*: Verify exit code 0 and usage of `timeout`, `width`, `height`, `margin`.

#### LauncherGuiTests (6 Cases)
13. **`test_launcher_set_width_valid`**
    - *Description*: Set application launcher window width (800).
    - *Assertions*: Verify `rofi/config.rasi` has `width: 800px;` under `window`.
14. **`test_launcher_set_icon_size_valid`**
    - *Description*: Set application launcher icon size (32).
    - *Assertions*: Verify `rofi/config.rasi` has `size: 32px;` under `element-icon`.
15. **`test_launcher_set_opacity_valid`**
    - *Description*: Set application launcher window opacity (85).
    - *Assertions*: Verify the alpha channel value of the `bg` variable is updated to `D9` (hex).
16. **`test_launcher_gui_launch`**
    - *Description*: Launch launcher GUI headlessly.
    - *Assertions*: Verify it runs, blocks, and can be terminated.
17. **`test_launcher_help`**
    - *Description*: Run launcher GUI with `--help`.
    - *Assertions*: Verify exit code 0 and help text matching `width`, `icon-size`, `opacity`.
18. **`test_launcher_multiple_valid_together`**
    - *Description*: Apply width, icon size, and opacity concurrently.
    - *Assertions*: Verify all three changes are written to `rofi/config.rasi`.

#### LockscreenGuiTests (6 Cases)
19. **`test_lockscreen_set_ring_color_valid`**
    - *Description*: Set lock screen ring-color (ff00ff).
    - *Assertions*: Verify `swaylock/config` has key `ring-color` set to `ff00ff`.
20. **`test_lockscreen_set_dim_time_valid`**
    - *Description*: Set dim timeout (120 seconds).
    - *Assertions*: Verify `labwc/autostart` has swayidle command timeout updated to `120` for dim/off action.
21. **`test_lockscreen_set_lock_time_valid`**
    - *Description*: Set lock screen timeout (240 seconds).
    - *Assertions*: Verify `labwc/autostart` has swayidle command timeout updated to `240` for swaylock action.
22. **`test_lockscreen_set_suspend_time_valid`**
    - *Description*: Set suspend timeout (480 seconds).
    - *Assertions*: Verify `labwc/autostart` has swayidle command timeout updated to `480` for suspend action.
23. **`test_lockscreen_gui_launch`**
    - *Description*: Run lock screen GUI headlessly.
    - *Assertions*: Verify it runs, blocks, and terminates.
24. **`test_lockscreen_help`**
    - *Description*: Run lock screen GUI with `--help`.
    - *Assertions*: Verify exit code 0 and presence of keywords `ring-color`, `dim-time`, `lock-time`, `suspend-time`.

#### ScreenshotGuiTests (6 Cases)
25. **`test_screenshot_set_save_dir_valid`**
    - *Description*: Set custom screenshot directory (/tmp/custom-pics).
    - *Assertions*: Verify `screenshot/config` has key `Save Directory` set to `/tmp/custom-pics`.
26. **`test_screenshot_set_delay_valid`**
    - *Description*: Set capture delay (5 seconds).
    - *Assertions*: Verify `screenshot/config` has key `Delay (s)` set to `5`.
27. **`test_screenshot_set_image_format_valid`**
    - *Description*: Set capture image format (jpg).
    - *Assertions*: Verify `screenshot/config` has key `Image Format` set to `jpg`.
28. **`test_screenshot_gui_launch`**
    - *Description*: Run screenshot GUI headlessly.
    - *Assertions*: Verify it runs, blocks, and terminates.
29. **`test_screenshot_help`**
    - *Description*: Run screenshot GUI with `--help`.
    - *Assertions*: Verify exit code 0 and presence of keywords `save-dir`, `delay`, `image-format`.
30. **`test_screenshot_multiple_valid_together`**
    - *Description*: Set save directory, delay, and format concurrently.
    - *Assertions*: Verify all three changes are written to `screenshot/config`.

---

### Tier 2: Boundary & Corner (15 Cases)
Validates system behavior on out-of-bounds inputs, invalid formats, and missing target structures.

#### WmGuiTests (3 Cases)
31. **`test_wm_set_gap_negative`**
    - *Description*: Attempt to set a negative gap (-5).
    - *Assertions*: Verify application rejects the input and exits with a non-zero exit code.
32. **`test_wm_set_border_width_excessive`**
    - *Description*: Attempt to set an excessively large border width (500).
    - *Assertions*: Verify application rejects the input and exits with a non-zero exit code.
33. **`test_wm_missing_config`**
    - *Description*: Remove labwc config directory prior to running tool.
    - *Assertions*: Verify the application recreates the directories and writes correct config settings.

#### NotificationsGuiTests (3 Cases)
34. **`test_notifications_set_timeout_invalid`**
    - *Description*: Attempt to set a negative timeout (-1).
    - *Assertions*: Verify application rejects input and exits with non-zero exit code.
35. **`test_notifications_set_margin_excessive`**
    - *Description*: Attempt to set an excessive margin (1000).
    - *Assertions*: Verify application rejects input and exits with non-zero exit code.
36. **`test_notifications_missing_config`**
    - *Description*: Remove Dunst configuration directory.
    - *Assertions*: Verify it is recreated with default values plus requested updates.

#### LauncherGuiTests (3 Cases)
37. **`test_launcher_set_width_percent`**
    - *Description*: Attempt to set launcher width as a percentage (110%).
    - *Assertions*: Verify application rejects non-pixel widths and exits with non-zero code.
38. **`test_launcher_set_opacity_out_of_bounds`**
    - *Description*: Set opacity to 150 (valid range is 0-100).
    - *Assertions*: Verify application rejects the value and exits with non-zero code.
39. **`test_launcher_missing_config`**
    - *Description*: Delete Rofi configurations directory.
    - *Assertions*: Verify the file `config.rasi` is recreated correctly with standard formatting.

#### LockscreenGuiTests (3 Cases)
40. **`test_lockscreen_invalid_color`**
    - *Description*: Set an invalid hex color name (e.g. `not-a-color`).
    - *Assertions*: Verify application rejects the input and exits with non-zero code.
41. **`test_lockscreen_dim_greater_than_lock`**
    - *Description*: Set dim time greater than lock time (dim=500, lock=300).
    - *Assertions*: Verify validation logic rejects the configuration combination (dim must occur before lock).
42. **`test_lockscreen_missing_config`**
    - *Description*: Delete swaylock and labwc configurations.
    - *Assertions*: Verify target configurations and autostart files are safely recreated.

#### ScreenshotGuiTests (3 Cases)
43. **`test_screenshot_set_delay_negative`**
    - *Description*: Attempt to set a negative screenshot delay (-3).
    - *Assertions*: Verify application rejects the value and exits with non-zero code.
44. **`test_screenshot_invalid_format`**
    - *Description*: Attempt to set an unsupported image format (e.g. `gif`).
    - *Assertions*: Verify application validation rejects the format and exits with non-zero code.
45. **`test_screenshot_missing_config`**
    - *Description*: Delete screenshot configurations.
    - *Assertions*: Verify screenshot configuration directory and file are recreated on command run.

---

### Tier 3: Cross-Feature Interaction (10 Cases)
Tests cascading parameters, sequential applications, and multi-process safety.

46. **`test_cross_wm_and_launcher_width`**
    - *Description*: Run `wm-gui` and `launcher-gui` sequentially.
    - *Assertions*: Verify each tool preserves the other's settings, and they do not corrupt configuration structures.
47. **`test_cross_lockscreen_autostart_preservation`**
    - *Description*: Modify lockscreen timeout parameters.
    - *Assertions*: Verify the write operation does not remove other startup items (e.g. dunst, copyq, swaybg) in `labwc/autostart`.
48. **`test_cross_notifications_and_screenshot`**
    - *Description*: Customize timeout via notifications-gui and save directory via screenshot-gui.
    - *Assertions*: Verify both target configurations write and persist independently.
49. **`test_cross_sequential_all_apps`**
    - *Description*: Run all 5 applications in sequence in the same sandbox environment.
    - *Assertions*: Verify all changes are successfully written and all files coexist correctly.
50. **`test_cross_multiple_help`**
    - *Description*: Sequentially request help flags on all five applications.
    - *Assertions*: Verify all applications return exit code 0.
51. **`test_cross_invalid_xdg_config_home`**
    - *Description*: Set `XDG_CONFIG_HOME` to a read-only or invalid directory.
    - *Assertions*: Verify applications handle file write failures gracefully and return non-zero exit codes.
52. **`test_cross_dunst_corner_radius_and_wm_corner_radius`**
    - *Description*: Adjust Dunst notification layout and WM layout.
    - *Assertions*: Verify corner radius properties do not conflict or overwrite each other.
53. **`test_cross_swaylock_color_nord_theme_sync`**
    - *Description*: Verify that lockscreen color changes coordinate correctly with Nord themed colors.
    - *Assertions*: Verify color writes matching active color variables.
54. **`test_cross_parallel_execution`**
    - *Description*: Trigger `wm-gui` and `launcher-gui` updates concurrently.
    - *Assertions*: Verify that concurrent file lock or write issues do not crash the utilities or corrupt configs.
55. **`test_cross_config_reload_trigger`**
    - *Description*: Verify that setting new configuration triggers config reloads (if configured) without crashing.
    - *Assertions*: Verify process exits normally.

---

### Tier 4: Real-World Workload Scenarios (5 Cases)
Simulates actual user setup pipelines, high-density settings changes, and recovery procedures.

56. **`test_realworld_initial_setup`**
    - *Description*: Brand-new system user configuration workflow where no target config directory exists.
    - *Assertions*: Run all five utilities. Verify that all folders are created and all keys match target setup parameters.
57. **`test_realworld_rapid_update_stream`**
    - *Description*: Simulate a user rapidly sliding configuration sliders (e.g. gaps adjustment).
    - *Assertions*: Run the configuration command 10 times in quick succession. Verify that the final gap value matches the last run and no files are corrupted.
58. **`test_realworld_corrupt_config_recovery`**
    - *Description*: Write corrupted/malformed configuration files (unclosed XML tags in `rc.xml`, syntactic errors in `dunstrc`).
    - *Assertions*: Run the tools to update a configuration key. Verify they rewrite or recover the file syntax and update keys properly.
59. **`test_realworld_theme_profile_deployment`**
    - *Description*: Apply a unified Nord-Dark design profile across WM and Lockscreen.
    - *Assertions*: Verify that gaps, corner radius, borders, and ring colors update to Nord standards.
60. **`test_realworld_session_migration`**
    - *Description*: Migrate system configs from one home workspace directory to another.
    - *Assertions*: Change `XDG_CONFIG_HOME` and write configs. Verify target directories are correctly populated.
