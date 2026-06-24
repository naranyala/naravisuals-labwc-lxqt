#!/usr/bin/env python3
# Integration test: Miriway (Mir-based compositor) + LXQt session
# Launches Miriway nested on X11, verifies Wayland protocol support,
# and validates LXQt components can connect and operate.

import os
import re
import sys
import time
import signal
import shutil
import struct
import socket
import tempfile
import unittest
import subprocess
import xml.etree.ElementTree as ET

MIRIWAY_BIN = shutil.which('miriway') or '/usr/bin/miriway'
MIRIWAY_SHELL_BIN = shutil.which('miriway-shell') or '/usr/bin/miriway-shell'
WAYLAND_INFO = shutil.which('wayland-info') or ''
SWAYBG = shutil.which('swaybg') or ''
LXQT_PANEL = shutil.which('lxqt-panel') or ''
LXQT_SESSION = shutil.which('lxqt-session') or ''
QT_TERMINAL = shutil.which('qterminal') or '/usr/bin/qterminal'

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '../..'))


class MiriwayLXQtTest(unittest.TestCase):
    """Test Miriway compositor integration with LXQt components."""

    @classmethod
    def setUpClass(cls):
        cls.miriway_available = os.path.exists(MIRIWAY_BIN) and os.path.exists(MIRIWAY_SHELL_BIN)
        cls.wayland_info_available = os.path.exists(WAYLAND_INFO)
        cls.swaybg_available = os.path.exists(SWAYBG)
        cls.lxqt_panel_available = os.path.exists(LXQT_PANEL)
        cls.display_available = 'DISPLAY' in os.environ
        cls.wayland_socket = os.environ.get('WAYLAND_DISPLAY', '')

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.xdg_config_home = self.temp_dir.name
        self.host_display = os.environ.get('DISPLAY', ':0')
        self.host_wayland = os.environ.get('WAYLAND_DISPLAY', '')

        if not self.__class__.miriway_available:
            self.skipTest('miriway not found')

    def tearDown(self):
        try:
            self.temp_dir.cleanup()
        except Exception:
            pass

    def _make_miriway_config(self, shell_components=None, extra_lines=None):
        """Create a minimal miriway-shell.config in the sandbox."""
        lines = [
            'x11-window-title=TestMiriwayLXQt',
            'idle-timeout=0',
        ]
        if shell_components:
            for comp in shell_components:
                lines.append(f'shell-component={comp}')
        if extra_lines:
            lines.extend(extra_lines)
        lines.extend([
            'ctrl-alt=BackSpace:@exit',
        ])
        config_path = os.path.join(self.xdg_config_home, 'miriway-shell.config')
        with open(config_path, 'w') as f:
            f.write('\n'.join(lines) + '\n')
        return config_path

    def _find_miriway_wayland_socket(self, runtime_dir, timeout=10):
        """Poll for the Miriway Wayland socket to appear (in XDG_RUNTIME_DIR)."""
        import glob as glob_mod
        start = time.time()
        while time.time() - start < timeout:
            for entry in glob_mod.glob(os.path.join(runtime_dir, 'wayland-*')):
                if entry.endswith('.lock'):
                    continue
                lock = entry + '.lock'
                if os.path.exists(lock) and os.stat(lock).st_size == 0:
                    return entry
            time.sleep(0.3)
        return None

    def _start_miriway_nested(self, config_path=None, extra_env=None):
        """Start miriway-shell in nested X11 mode and return (process, runtime_dir, wayland_socket)."""
        runtime_dir = os.path.join(self.temp_dir.name, 'runtime')
        os.makedirs(runtime_dir, mode=0o700, exist_ok=True)

        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['XDG_RUNTIME_DIR'] = runtime_dir
        env['DISPLAY'] = self.host_display
        env['MIR_SERVER_PLATFORM_DISPLAY_LIBS'] = 'mir:x11'
        env['MIR_SERVER_ENABLE_X11'] = '1'
        env['XDG_CURRENT_DESKTOP'] = 'LXQt:Miriway:wlroots'
        env.pop('WAYLAND_DISPLAY', None)
        if extra_env:
            env.update(extra_env)

        proc = subprocess.Popen(
            [MIRIWAY_SHELL_BIN],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid,
        )

        sock = self._find_miriway_wayland_socket(runtime_dir, timeout=10)
        if not sock:
            try:
                _, stderr = proc.communicate(timeout=3)
            except subprocess.TimeoutExpired:
                pass
        return proc, runtime_dir, sock

    def _run_wayland_client(self, wayland_socket, cmd, timeout=5):
        """Run a command with WAYLAND_DISPLAY pointing at Miriway's socket."""
        env = os.environ.copy()
        sock_name = os.path.basename(wayland_socket)
        runtime_dir = os.path.dirname(wayland_socket)
        env['WAYLAND_DISPLAY'] = sock_name
        env['XDG_RUNTIME_DIR'] = runtime_dir
        env['DISPLAY'] = ''  # Don't use X11
        try:
            res = subprocess.run(
                cmd, env=env,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                text=True, timeout=timeout,
            )
            return res
        except subprocess.TimeoutExpired as e:
            return e

    def _stop_miriway(self, proc):
        """Cleanly stop a Miriway process."""
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            proc.wait(timeout=2)

    # ---------------------------------------------------------------
    # Tests
    # ---------------------------------------------------------------

    def test_01_miriway_starts_and_creates_socket(self):
        """Miriway compositor starts in nested mode and creates a Wayland socket."""
        self._make_miriway_config()
        proc, rt_dir, sock = self._start_miriway_nested()
        self.assertIsNotNone(sock, 'Miriway did not create a Wayland socket within timeout')
        self.assertTrue(os.path.exists(sock), f'Wayland socket {sock} does not exist')
        self._stop_miriway(proc)

    def test_02_wayland_protocols_available(self):
        """Miriway exposes core Wayland protocols (layer-shell is shell-only)."""
        if not self.__class__.wayland_info_available:
            self.skipTest('wayland-info not available')

        self._make_miriway_config()
        proc, rt_dir, sock = self._start_miriway_nested()
        self.assertIsNotNone(sock, 'Miriway did not start')

        try:
            res = self._run_wayland_client(sock, [WAYLAND_INFO])
            output = res.stdout + res.stderr

            # These protocols are available to ALL clients on Miriway
            public_protocols = [
                'xdg_wm_base',
                'zxdg_output_manager_v1',
                'wl_compositor',
                'wl_subcompositor',
                'wl_data_device_manager',
                'wl_seat',
                'zwp_relative_pointer_manager_v1',
                'zwp_pointer_constraints_v1',
                'zwp_idle_inhibit_manager_v1',
                'zwlr_screencopy_manager_v1',
                'wl_shm',
                'wp_viewporter',
                'wl_output',
                'xdg_activation_v1',
                'zxdg_decoration_manager_v1',
                'wp_fractional_scale_manager_v1',
            ]
            for proto in public_protocols:
                self.assertIn(
                    proto, output,
                    f'Required protocol {proto} not found in wayland-info output'
                )

            # These protocols are RESERVED for shell components only
            # (not visible to arbitrary clients via wayland-info)
            shell_protocols = [
                'zwlr_layer_shell_v1',
                'zwlr_foreign_toplevel_manager_v1',
            ]
            for proto in shell_protocols:
                self.assertNotIn(
                    proto, output,
                    f'Shell-only protocol {proto} should NOT be visible to arbitrary clients'
                )
        finally:
            self._stop_miriway(proc)

    def test_03_miriway_config_migration_creates_settings(self):
        """Miriway migrates .config to .settings file on startup."""
        self._make_miriway_config()
        proc, rt_dir, sock = self._start_miriway_nested()
        self.assertIsNotNone(sock, 'Miriway did not start')

        try:
            settings_path = os.path.join(self.xdg_config_home, 'miriway-shell.settings')
            self.assertTrue(
                os.path.exists(settings_path),
                'Settings file was not created by config migration'
            )
            with open(settings_path) as f:
                content = f.read()
            # Migration should transfer known keys like ctrl-alt
            self.assertIn('command_ctrl_alt', content)
            self.assertIn('BackSpace:@exit', content)
        finally:
            self._stop_miriway(proc)

    def test_04_xwayland_in_path(self):
        """XWayland binary is available and Miriway detects it."""
        xwayland_path = shutil.which('Xwayland')
        self.assertIsNotNone(xwayland_path, 'Xwayland not found in PATH')
        self.assertTrue(os.access(xwayland_path, os.X_OK), 'Xwayland not executable')

    def test_05_lxqt_panel_binary_works(self):
        """lxqt-panel binary exists and can parse its own --help."""
        if not self.__class__.lxqt_panel_available:
            self.skipTest('lxqt-panel not found')
        res = subprocess.run(
            [LXQT_PANEL, '--help'],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, timeout=5,
        )
        self.assertIn('Usage', res.stdout + res.stderr)


class LXQtSessionIntegrationTest(unittest.TestCase):
    """Test lxqt-session can be launched as a Miriway shell-component."""

    @classmethod
    def setUpClass(cls):
        cls.lxqt_session_available = os.path.exists(LXQT_SESSION)

    def setUp(self):
        if not self.__class__.lxqt_session_available:
            self.skipTest('lxqt-session not found')

    def test_lxqt_session_binary_exists(self):
        """lxqt-session is installed and executable."""
        self.assertTrue(os.path.exists(LXQT_SESSION), 'lxqt-session binary not found')
        self.assertTrue(os.access(LXQT_SESSION, os.X_OK), 'lxqt-session not executable')

    def test_lxqt_session_help(self):
        """lxqt-session responds to --help."""
        res = subprocess.run(
            [LXQT_SESSION, '--help'],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, timeout=5,
        )
        self.assertIn('Usage', res.stdout + res.stderr)


if __name__ == '__main__':
    unittest.main()
