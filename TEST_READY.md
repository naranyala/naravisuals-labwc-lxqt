# Naravisuals Labwc LXQt E2E Test Suite - Readiness & Verification

This document details how to run the E2E test suite, the expected initial execution outputs, and contains a checklist of all 60 test cases.

---

## 1. Test Runner Invocation

### Method A: Using CTest (Recommended)
From the project root directory, run:
```bash
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
```

To run a specific test suite via CTest:
```bash
ctest -R e2e_wm_gui --output-on-failure
```

### Method B: Direct Python Execution
You can also invoke the test runner directly with Python to run specific test suites. Make sure to specify the built executable paths:
```bash
python3 tests/e2e/e2e_test_runner.py WmGuiTests \
  --wm-gui build/apps/wm-gui/nv-wm-gui
```

---

## 2. Expected Output (Initial Run)

Since the target Qt6 applications currently contain only placeholder graphical user interfaces without backend configuration saving logic or CLI command-line argument parsing, **all E2E tests are expected to FAIL or timeout initially**.

The output of a typical failed run of `ctest` will look like:
```text
Test project /media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt/build
      Start  1: e2e_wm_gui
 1/7 Test  #1: e2e_wm_gui ........................***Failed    5.20 sec
      Start  2: e2e_notifications_gui
 2/7 Test  #2: e2e_notifications_gui .............***Failed    5.22 sec
      Start  3: e2e_launcher_gui
 3/7 Test  #3: e2e_launcher_gui ..................***Failed    5.21 sec
      Start  4: e2e_lockscreen_gui
 4/7 Test  #4: e2e_lockscreen_gui ................***Failed    5.22 sec
      Start  5: e2e_screenshot_gui
 5/7 Test  #5: e2e_screenshot_gui ................***Failed    5.22 sec
      Start  6: e2e_cross_feature
 6/7 Test  #6: e2e_cross_feature .................***Failed    7.12 sec
      Start  7: e2e_real_world
 7/7 Test  #7: e2e_real_world ....................***Failed    5.30 sec

0% tests passed, 7 tests failed out of 7

Total Test time (real) =  38.49 sec

The following tests FAILED:
	  1 - e2e_wm_gui (Failed)
	  2 - e2e_notifications_gui (Failed)
	  3 - e2e_launcher_gui (Failed)
	  4 - e2e_lockscreen_gui (Failed)
	  5 - e2e_screenshot_gui (Failed)
	  6 - e2e_cross_feature (Failed)
	  7 - e2e_real_world (Failed)
Errors were encountered while processing tests.
```

---

## 3. Test Cases Checklist (60 Cases)

### Tier 1: Feature Coverage (30 Cases)
#### WmGuiTests
- [ ] `test_wm_set_gap_valid` — Sets gap to 12 in `labwc/rc.xml`
- [ ] `test_wm_set_corner_radius_valid` — Sets corner radius to 15 in `labwc/rc.xml`
- [ ] `test_wm_set_border_width_valid` — Sets border width to 4 in `labwc/themerc-override`
- [ ] `test_wm_gui_launch` — Verifies normal GUI headless launch blocks properly
- [ ] `test_wm_help` — Checks `--help` flag output and exit status
- [ ] `test_wm_multiple_valid_together` — Sets gap, corner radius, and border width concurrently

#### NotificationsGuiTests
- [ ] `test_notifications_set_timeout_valid` — Sets Dunst timeout to 15 in `dunst/dunstrc`
- [ ] `test_notifications_set_width_valid` — Sets Dunst width to 450 in `dunst/dunstrc`
- [ ] `test_notifications_set_height_valid` — Sets Dunst height to 150 in `dunst/dunstrc`
- [ ] `test_notifications_set_margin_valid` — Sets Dunst margin to 25 in `dunst/dunstrc`
- [ ] `test_notifications_gui_launch` — Verifies normal Dunst GUI headless launch blocks
- [ ] `test_notifications_help` — Checks notifications GUI `--help` output

#### LauncherGuiTests
- [ ] `test_launcher_set_width_valid` — Sets Rofi width to 800px in `rofi/config.rasi`
- [ ] `test_launcher_set_icon_size_valid` — Sets Rofi icon size to 32px in `rofi/config.rasi`
- [ ] `test_launcher_set_opacity_valid` — Sets Rofi window background alpha channel to D9 (85% opacity)
- [ ] `test_launcher_gui_launch` — Verifies Rofi GUI headless launch blocks
- [ ] `test_launcher_help` — Checks launcher GUI `--help` output
- [ ] `test_launcher_multiple_valid_together` — Sets width, icon size, and opacity concurrently

#### LockscreenGuiTests
- [ ] `test_lockscreen_set_ring_color_valid` — Sets ring color to ff00ff in `swaylock/config`
- [ ] `test_lockscreen_set_dim_time_valid` — Sets swayidle dim timeout to 120 in `labwc/autostart`
- [ ] `test_lockscreen_set_lock_time_valid` — Sets swayidle lock timeout to 240 in `labwc/autostart`
- [ ] `test_lockscreen_set_suspend_time_valid` — Sets swayidle suspend timeout to 480 in `labwc/autostart`
- [ ] `test_lockscreen_gui_launch` — Verifies Lockscreen GUI headless launch blocks
- [ ] `test_lockscreen_help` — Checks lockscreen GUI `--help` output

#### ScreenshotGuiTests
- [ ] `test_screenshot_set_save_dir_valid` — Sets save directory to /tmp/custom-pics in `screenshot/config`
- [ ] `test_screenshot_set_delay_valid` — Sets delay to 5 in `screenshot/config`
- [ ] `test_screenshot_set_image_format_valid` — Sets image format to jpg in `screenshot/config`
- [ ] `test_screenshot_gui_launch` — Verifies Screenshot GUI headless launch blocks
- [ ] `test_screenshot_help` — Checks screenshot GUI `--help` output
- [ ] `test_screenshot_multiple_valid_together` — Sets save directory, delay, and format concurrently

### Tier 2: Boundary & Corner (15 Cases)
#### WmGuiTests
- [ ] `test_wm_set_gap_negative` — Verifies negative gaps are rejected
- [ ] `test_wm_set_border_width_excessive` — Verifies excessive border widths are rejected
- [ ] `test_wm_missing_config` — Recreates config directory safely if deleted

#### NotificationsGuiTests
- [ ] `test_notifications_set_timeout_invalid` — Verifies negative timeout values are rejected
- [ ] `test_notifications_set_margin_excessive` — Verifies excessive margin values are rejected
- [ ] `test_notifications_missing_config` — Safely rebuilds dunstrc file if missing

#### LauncherGuiTests
- [ ] `test_launcher_set_width_percent` — Rejects percent values for window width
- [ ] `test_launcher_set_opacity_out_of_bounds` — Rejects opacity values outside the [0, 100] range
- [ ] `test_launcher_missing_config` — Rebuilds config.rasi file safely if deleted

#### LockscreenGuiTests
- [ ] `test_lockscreen_invalid_color` — Rejects non-hex ring color values
- [ ] `test_lockscreen_dim_greater_than_lock` — Rejects lock time values smaller than dim time
- [ ] `test_lockscreen_missing_config` — Rebuilds autostart and swaylock configs if deleted

#### ScreenshotGuiTests
- [ ] `test_screenshot_set_delay_negative` — Rejects negative delays
- [ ] `test_screenshot_invalid_format` — Rejects unsupported image formats
- [ ] `test_screenshot_missing_config` — Recreates screenshot config file if deleted

### Tier 3: Cross-Feature Interaction (10 Cases)
- [ ] `test_cross_wm_and_launcher_width` — Assures sequential settings updates do not corrupt configs
- [ ] `test_cross_lockscreen_autostart_preservation` — Modifying timeouts keeps other autostart items intact
- [ ] `test_cross_notifications_and_screenshot` — Independent dunst and screenshot configuration updates
- [ ] `test_cross_sequential_all_apps` — Verifies complete desktop setting customization sequentially
- [ ] `test_cross_multiple_help` — Runs help checks sequentially across all five applications
- [ ] `test_cross_invalid_xdg_config_home` — Gracefully handles errors when XDG_CONFIG_HOME is read-only
- [ ] `test_cross_dunst_corner_radius_and_wm_corner_radius` — Keeps corner radius values separate and correct
- [ ] `test_cross_swaylock_color_nord_theme_sync` — Checks color matching Nord theme configurations
- [ ] `test_cross_parallel_execution` — Concurrently modifies settings without deadlock or file corruption
- [ ] `test_cross_config_reload_trigger` — Runs reconfigure signals safely

### Tier 4: Real-World Workload Scenarios (5 Cases)
- [ ] `test_realworld_initial_setup` — Simulates first-time login config creation and application customization
- [ ] `test_realworld_rapid_update_stream` — Simulates rapid UI slider setting updates
- [ ] `test_realworld_corrupt_config_recovery` — Overwrites and recovers syntactically malformed configs
- [ ] `test_realworld_theme_profile_deployment` — Applies a complete Nord-Dark theme profile smoothly
- [ ] `test_realworld_session_migration` — Switches workspace paths using custom XDG_CONFIG_HOME overrides
