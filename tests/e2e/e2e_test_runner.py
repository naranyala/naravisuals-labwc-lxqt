import os
import re
import sys
import shutil
import tempfile
import argparse
import unittest
import subprocess
import xml.etree.ElementTree as ET
import configparser

# Parse custom arguments
parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('--wm-gui', default=None)
parser.add_argument('--notifications-gui', default=None)
parser.add_argument('--launcher-gui', default=None)
parser.add_argument('--lockscreen-gui', default=None)
parser.add_argument('--screenshot-gui', default=None)

args, remaining_argv = parser.parse_known_args()

EXECUTABLES = {
    'wm-gui': args.wm_gui,
    'notifications-gui': args.notifications_gui,
    'launcher-gui': args.launcher_gui,
    'lockscreen-gui': args.lockscreen_gui,
    'screenshot-gui': args.screenshot_gui
}

# Adjust sys.argv for unittest
sys.argv = [sys.argv[0]] + remaining_argv

class BaseE2ETest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.xdg_config_home = self.temp_dir.name
        
        # Determine the project root dynamically
        self.project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '../..'))
        self.src_configs = os.path.join(self.project_root, 'configs/dotfiles')
        
        # Copy template configuration files to our temporary sandbox
        if os.path.exists(self.src_configs):
            shutil.copytree(self.src_configs, self.xdg_config_home, dirs_exist_ok=True)
            
    def tearDown(self):
        try:
            self.temp_dir.cleanup()
        except Exception:
            pass
        
    def get_bin(self, app_name):
        exe = EXECUTABLES.get(app_name)
        if not exe or not os.path.exists(exe):
            self.skipTest(f"Executable for {app_name} not found or not provided: {exe}")
        return exe

    def run_app(self, app_name, args, expected_code=0, timeout=3):
        exe = self.get_bin(app_name)
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        
        cmd = [exe] + args
        try:
            res = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=timeout)
            self.assertEqual(res.returncode, expected_code, f"App {app_name} exited with {res.returncode}, expected {expected_code}. Stderr: {res.stderr}")
            return res
        except subprocess.TimeoutExpired as e:
            self.fail(f"Application {app_name} timed out with arguments {args}")

    def run_app_gui_only(self, app_name, timeout=2):
        exe = self.get_bin(app_name)
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        
        proc = subprocess.Popen([exe], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
            self.fail(f"GUI mode did not block as expected, exited with {proc.returncode}. Stderr: {stderr}")
        except subprocess.TimeoutExpired:
            proc.terminate()
            try:
                proc.wait(timeout=1)
            except subprocess.TimeoutExpired:
                proc.kill()


class WmGuiTests(BaseE2ETest):
    # --- Tier 1: Feature Coverage ---
    def test_wm_set_gap_valid(self):
        self.run_app('wm-gui', ['--set-gaps', '12'])
        path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        self.assertTrue(os.path.exists(path))
        tree = ET.parse(path)
        root = tree.getroot()
        gap = root.find('.//core/gap')
        self.assertIsNotNone(gap)
        self.assertEqual(gap.text, '12')

    def test_wm_set_corner_radius_valid(self):
        self.run_app('wm-gui', ['--set-corner-radius', '15'])
        path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        self.assertTrue(os.path.exists(path))
        tree = ET.parse(path)
        root = tree.getroot()
        cr = root.find('.//theme/cornerRadius')
        self.assertIsNotNone(cr)
        self.assertEqual(cr.text, '15')

    def test_wm_set_border_width_valid(self):
        self.run_app('wm-gui', ['--set-border-width', '4'])
        path = os.path.join(self.xdg_config_home, 'labwc/themerc-override')
        self.assertTrue(os.path.exists(path))
        with open(path, 'r') as f:
            content = f.read()
        self.assertIn('window.border.width: 4', content)

    def test_wm_gui_launch(self):
        self.run_app_gui_only('wm-gui')

    def test_wm_help(self):
        res = self.run_app('wm-gui', ['--help'])
        self.assertIn('gap', res.stdout.lower() + res.stderr.lower())
        self.assertIn('corner-radius', res.stdout.lower() + res.stderr.lower())
        self.assertIn('border-width', res.stdout.lower() + res.stderr.lower())

    def test_wm_multiple_valid_together(self):
        self.run_app('wm-gui', ['--set-gaps', '16', '--set-corner-radius', '8', '--set-border-width', '3'])
        
        rc_path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        tree = ET.parse(rc_path)
        root = tree.getroot()
        gap = root.find('.//core/gap')
        cr = root.find('.//theme/cornerRadius')
        self.assertIsNotNone(gap)
        self.assertEqual(gap.text, '16')
        self.assertIsNotNone(cr)
        self.assertEqual(cr.text, '8')
        
        override_path = os.path.join(self.xdg_config_home, 'labwc/themerc-override')
        with open(override_path, 'r') as f:
            content = f.read()
        self.assertIn('window.border.width: 3', content)

    # --- Tier 2: Boundary & Corner ---
    def test_wm_set_gap_negative(self):
        exe = self.get_bin('wm-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-gaps', '-5'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_wm_set_border_width_excessive(self):
        exe = self.get_bin('wm-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-border-width', '500'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_wm_missing_config(self):
        shutil.rmtree(os.path.join(self.xdg_config_home, 'labwc'), ignore_errors=True)
        self.run_app('wm-gui', ['--set-gaps', '10'])
        rc_path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        self.assertTrue(os.path.exists(rc_path))
        tree = ET.parse(rc_path)
        root = tree.getroot()
        gap = root.find('.//core/gap')
        self.assertIsNotNone(gap)
        self.assertEqual(gap.text, '10')


class NotificationsGuiTests(BaseE2ETest):
    # --- Tier 1: Feature Coverage ---
    def test_notifications_set_timeout_valid(self):
        self.run_app('notifications-gui', ['--set-timeout', '15'])
        path = os.path.join(self.xdg_config_home, 'dunst/dunstrc')
        self.assertTrue(os.path.exists(path))
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('global', 'timeout'), '15')

    def test_notifications_set_width_valid(self):
        self.run_app('notifications-gui', ['--set-width', '450'])
        path = os.path.join(self.xdg_config_home, 'dunst/dunstrc')
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('global', 'width'), '450')

    def test_notifications_set_height_valid(self):
        self.run_app('notifications-gui', ['--set-height', '150'])
        path = os.path.join(self.xdg_config_home, 'dunst/dunstrc')
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('global', 'height'), '150')

    def test_notifications_set_margin_valid(self):
        self.run_app('notifications-gui', ['--set-margin', '25'])
        path = os.path.join(self.xdg_config_home, 'dunst/dunstrc')
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('global', 'margin'), '25')

    def test_notifications_gui_launch(self):
        self.run_app_gui_only('notifications-gui')

    def test_notifications_help(self):
        res = self.run_app('notifications-gui', ['--help'])
        self.assertIn('timeout', res.stdout.lower() + res.stderr.lower())
        self.assertIn('width', res.stdout.lower() + res.stderr.lower())
        self.assertIn('height', res.stdout.lower() + res.stderr.lower())
        self.assertIn('margin', res.stdout.lower() + res.stderr.lower())

    # --- Tier 2: Boundary & Corner ---
    def test_notifications_set_timeout_invalid(self):
        exe = self.get_bin('notifications-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-timeout', '-1'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_notifications_set_margin_excessive(self):
        exe = self.get_bin('notifications-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-margin', '1000'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_notifications_missing_config(self):
        shutil.rmtree(os.path.join(self.xdg_config_home, 'dunst'), ignore_errors=True)
        self.run_app('notifications-gui', ['--set-timeout', '8'])
        path = os.path.join(self.xdg_config_home, 'dunst/dunstrc')
        self.assertTrue(os.path.exists(path))
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('global', 'timeout'), '8')


class LauncherGuiTests(BaseE2ETest):
    # --- Tier 1: Feature Coverage ---
    def test_launcher_set_width_valid(self):
        self.run_app('launcher-gui', ['--set-width', '800'])
        path = os.path.join(self.xdg_config_home, 'rofi/config.rasi')
        self.assertTrue(os.path.exists(path))
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'window\s*\{[^}]*width\s*:\s*(\d+)px', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1), '800')

    def test_launcher_set_icon_size_valid(self):
        self.run_app('launcher-gui', ['--set-icon-size', '32'])
        path = os.path.join(self.xdg_config_home, 'rofi/config.rasi')
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'element-icon\s*\{[^}]*size\s*:\s*(\d+)px', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1), '32')

    def test_launcher_set_opacity_valid(self):
        self.run_app('launcher-gui', ['--set-opacity', '85'])
        path = os.path.join(self.xdg_config_home, 'rofi/config.rasi')
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'bg\s*:\s*#[0-9a-fA-F]{6}([0-9a-fA-F]{2})', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1).upper(), 'D9')

    def test_launcher_gui_launch(self):
        self.run_app_gui_only('launcher-gui')

    def test_launcher_help(self):
        res = self.run_app('launcher-gui', ['--help'])
        self.assertIn('width', res.stdout.lower() + res.stderr.lower())
        self.assertIn('icon-size', res.stdout.lower() + res.stderr.lower())
        self.assertIn('opacity', res.stdout.lower() + res.stderr.lower())

    def test_launcher_multiple_valid_together(self):
        self.run_app('launcher-gui', ['--set-width', '700', '--set-icon-size', '48', '--set-opacity', '90'])
        path = os.path.join(self.xdg_config_home, 'rofi/config.rasi')
        with open(path, 'r') as f:
            content = f.read()
        
        match_w = re.search(r'window\s*\{[^}]*width\s*:\s*(\d+)px', content)
        self.assertIsNotNone(match_w)
        self.assertEqual(match_w.group(1), '700')
        
        match_i = re.search(r'element-icon\s*\{[^}]*size\s*:\s*(\d+)px', content)
        self.assertIsNotNone(match_i)
        self.assertEqual(match_i.group(1), '48')
        
        match_o = re.search(r'bg\s*:\s*#[0-9a-fA-F]{6}([0-9a-fA-F]{2})', content)
        self.assertIsNotNone(match_o)
        self.assertIn(match_o.group(1).upper(), ['E5', 'E6'])

    # --- Tier 2: Boundary & Corner ---
    def test_launcher_set_width_percent(self):
        exe = self.get_bin('launcher-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-width', '110%'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_launcher_set_opacity_out_of_bounds(self):
        exe = self.get_bin('launcher-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-opacity', '150'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_launcher_missing_config(self):
        shutil.rmtree(os.path.join(self.xdg_config_home, 'rofi'), ignore_errors=True)
        self.run_app('launcher-gui', ['--set-width', '500'])
        path = os.path.join(self.xdg_config_home, 'rofi/config.rasi')
        self.assertTrue(os.path.exists(path))
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'window\s*\{[^}]*width\s*:\s*(\d+)px', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1), '500')


class LockscreenGuiTests(BaseE2ETest):
    # --- Tier 1: Feature Coverage ---
    def test_lockscreen_set_ring_color_valid(self):
        self.run_app('lockscreen-gui', ['--set-ring-color', 'ff00ff'])
        path = os.path.join(self.xdg_config_home, 'swaylock/config')
        self.assertTrue(os.path.exists(path))
        with open(path, 'r') as f:
            content = f.read()
        self.assertIn('ring-color=ff00ff', content)

    def test_lockscreen_set_dim_time_valid(self):
        self.run_app('lockscreen-gui', ['--set-dim-time', '120'])
        path = os.path.join(self.xdg_config_home, 'labwc/autostart')
        self.assertTrue(os.path.exists(path))
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'timeout\s+(\d+)\s+\'(?:[^\']*wlopm --off[^\']*)\'', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1), '120')

    def test_lockscreen_set_lock_time_valid(self):
        self.run_app('lockscreen-gui', ['--set-lock-time', '240'])
        path = os.path.join(self.xdg_config_home, 'labwc/autostart')
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'timeout\s+(\d+)\s+\'(?:[^\']*swaylock -f[^\']*)\'', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1), '240')

    def test_lockscreen_set_suspend_time_valid(self):
        self.run_app('lockscreen-gui', ['--set-suspend-time', '480'])
        path = os.path.join(self.xdg_config_home, 'labwc/autostart')
        with open(path, 'r') as f:
            content = f.read()
        match = re.search(r'timeout\s+(\d+)\s+\'(?:[^\']*systemctl suspend[^\']*)\'', content)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1), '480')

    def test_lockscreen_gui_launch(self):
        self.run_app_gui_only('lockscreen-gui')

    def test_lockscreen_help(self):
        res = self.run_app('lockscreen-gui', ['--help'])
        self.assertIn('ring-color', res.stdout.lower() + res.stderr.lower())
        self.assertIn('dim-time', res.stdout.lower() + res.stderr.lower())
        self.assertIn('lock-time', res.stdout.lower() + res.stderr.lower())
        self.assertIn('suspend-time', res.stdout.lower() + res.stderr.lower())

    # --- Tier 2: Boundary & Corner ---
    def test_lockscreen_invalid_color(self):
        exe = self.get_bin('lockscreen-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-ring-color', 'not-a-color'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_lockscreen_dim_greater_than_lock(self):
        exe = self.get_bin('lockscreen-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-dim-time', '500', '--set-lock-time', '300'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_lockscreen_missing_config(self):
        shutil.rmtree(os.path.join(self.xdg_config_home, 'swaylock'), ignore_errors=True)
        autostart_file = os.path.join(self.xdg_config_home, 'labwc/autostart')
        if os.path.exists(autostart_file):
            os.remove(autostart_file)
        self.run_app('lockscreen-gui', ['--set-ring-color', '00ff00', '--set-lock-time', '300'])
        
        path_sway = os.path.join(self.xdg_config_home, 'swaylock/config')
        self.assertTrue(os.path.exists(path_sway))
        with open(path_sway, 'r') as f:
            self.assertIn('ring-color=00ff00', f.read())
            
        path_auto = os.path.join(self.xdg_config_home, 'labwc/autostart')
        self.assertTrue(os.path.exists(path_auto))
        with open(path_auto, 'r') as f:
            self.assertIn('timeout 300', f.read())


class ScreenshotGuiTests(BaseE2ETest):
    # --- Tier 1: Feature Coverage ---
    def test_screenshot_set_save_dir_valid(self):
        self.run_app('screenshot-gui', ['--set-save-dir', '/tmp/custom-pics'])
        path = os.path.join(self.xdg_config_home, 'screenshot/config')
        self.assertTrue(os.path.exists(path))
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('Screenshot', 'Save Directory'), '/tmp/custom-pics')

    def test_screenshot_set_delay_valid(self):
        self.run_app('screenshot-gui', ['--set-delay', '5'])
        path = os.path.join(self.xdg_config_home, 'screenshot/config')
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('Screenshot', 'Delay (s)'), '5')

    def test_screenshot_set_image_format_valid(self):
        self.run_app('screenshot-gui', ['--set-image-format', 'jpg'])
        path = os.path.join(self.xdg_config_home, 'screenshot/config')
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('Screenshot', 'Image Format'), 'jpg')

    def test_screenshot_gui_launch(self):
        self.run_app_gui_only('screenshot-gui')

    def test_screenshot_help(self):
        res = self.run_app('screenshot-gui', ['--help'])
        self.assertIn('save-dir', res.stdout.lower() + res.stderr.lower())
        self.assertIn('delay', res.stdout.lower() + res.stderr.lower())
        self.assertIn('image-format', res.stdout.lower() + res.stderr.lower())

    def test_screenshot_multiple_valid_together(self):
        self.run_app('screenshot-gui', ['--set-save-dir', '/tmp/screenshots', '--set-delay', '3', '--set-image-format', 'png'])
        path = os.path.join(self.xdg_config_home, 'screenshot/config')
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('Screenshot', 'Save Directory'), '/tmp/screenshots')
        self.assertEqual(config.get('Screenshot', 'Delay (s)'), '3')
        self.assertEqual(config.get('Screenshot', 'Image Format'), 'png')

    # --- Tier 2: Boundary & Corner ---
    def test_screenshot_set_delay_negative(self):
        exe = self.get_bin('screenshot-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-delay', '-3'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_screenshot_invalid_format(self):
        exe = self.get_bin('screenshot-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-image-format', 'gif'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_screenshot_missing_config(self):
        conf_dir = os.path.join(self.xdg_config_home, 'screenshot')
        shutil.rmtree(conf_dir, ignore_errors=True)
        self.run_app('screenshot-gui', ['--set-delay', '2'])
        path = os.path.join(self.xdg_config_home, 'screenshot/config')
        self.assertTrue(os.path.exists(path))
        config = configparser.ConfigParser()
        config.read(path)
        self.assertEqual(config.get('Screenshot', 'Delay (s)'), '2')


class CrossFeatureTests(BaseE2ETest):
    # --- Tier 3: Cross-Feature Interaction ---
    def test_cross_wm_and_launcher_width(self):
        self.run_app('wm-gui', ['--set-gaps', '15'])
        self.run_app('launcher-gui', ['--set-width', '750'])
        
        rc_path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        tree = ET.parse(rc_path)
        self.assertEqual(tree.getroot().find('.//core/gap').text, '15')
        
        rofi_path = os.path.join(self.xdg_config_home, 'rofi/config.rasi')
        with open(rofi_path, 'r') as f:
            self.assertIn('width:            750px;', f.read())

    def test_cross_lockscreen_autostart_preservation(self):
        autostart_path = os.path.join(self.xdg_config_home, 'labwc/autostart')
        with open(autostart_path, 'r') as f:
            original_content = f.read()
        self.assertIn('copyq --start-server', original_content)
        
        self.run_app('lockscreen-gui', ['--set-lock-time', '300'])
        
        with open(autostart_path, 'r') as f:
            new_content = f.read()
        self.assertIn('copyq --start-server', new_content)
        self.assertIn('timeout 300', new_content)

    def test_cross_notifications_and_screenshot(self):
        self.run_app('notifications-gui', ['--set-timeout', '12'])
        self.run_app('screenshot-gui', ['--set-save-dir', '/tmp/test_ss'])
        
        dunst_config = configparser.ConfigParser()
        dunst_config.read(os.path.join(self.xdg_config_home, 'dunst/dunstrc'))
        self.assertEqual(dunst_config.get('global', 'timeout'), '12')
        
        ss_config = configparser.ConfigParser()
        ss_config.read(os.path.join(self.xdg_config_home, 'screenshot/config'))
        self.assertEqual(ss_config.get('Screenshot', 'Save Directory'), '/tmp/test_ss')

    def test_cross_sequential_all_apps(self):
        self.run_app('wm-gui', ['--set-gaps', '10', '--set-corner-radius', '12', '--set-border-width', '3'])
        self.run_app('notifications-gui', ['--set-timeout', '8', '--set-width', '350'])
        self.run_app('launcher-gui', ['--set-width', '500', '--set-opacity', '95'])
        self.run_app('lockscreen-gui', ['--set-ring-color', 'ffffff', '--set-dim-time', '180'])
        self.run_app('screenshot-gui', ['--set-save-dir', '/home/user/Pictures', '--set-delay', '4'])
        
        # 1. WM
        tree = ET.parse(os.path.join(self.xdg_config_home, 'labwc/rc.xml'))
        self.assertEqual(tree.getroot().find('.//core/gap').text, '10')
        self.assertEqual(tree.getroot().find('.//theme/cornerRadius').text, '12')
        with open(os.path.join(self.xdg_config_home, 'labwc/themerc-override'), 'r') as f:
            self.assertIn('window.border.width: 3', f.read())
            
        # 2. Notifications
        config_d = configparser.ConfigParser()
        config_d.read(os.path.join(self.xdg_config_home, 'dunst/dunstrc'))
        self.assertEqual(config_d.get('global', 'timeout'), '8')
        self.assertEqual(config_d.get('global', 'width'), '350')
        
        # 3. Launcher
        with open(os.path.join(self.xdg_config_home, 'rofi/config.rasi'), 'r') as f:
            r_content = f.read()
        self.assertIn('width:            500px;', r_content)
        self.assertIn('bg:         #2e3440F2;', r_content)
        
        # 4. Lockscreen
        with open(os.path.join(self.xdg_config_home, 'swaylock/config'), 'r') as f:
            self.assertIn('ring-color=ffffff', f.read())
        with open(os.path.join(self.xdg_config_home, 'labwc/autostart'), 'r') as f:
            self.assertIn('timeout 180', f.read())
            
        # 5. Screenshot
        config_s = configparser.ConfigParser()
        config_s.read(os.path.join(self.xdg_config_home, 'screenshot/config'))
        self.assertEqual(config_s.get('Screenshot', 'Save Directory'), '/home/user/Pictures')
        self.assertEqual(config_s.get('Screenshot', 'Delay (s)'), '4')

    def test_cross_multiple_help(self):
        for app in ['wm-gui', 'notifications-gui', 'launcher-gui', 'lockscreen-gui', 'screenshot-gui']:
            self.run_app(app, ['--help'])

    def test_cross_invalid_xdg_config_home(self):
        exe = self.get_bin('wm-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = '/root/nonexistent_denied_directory'
        env['QT_QPA_PLATFORM'] = 'offscreen'
        res = subprocess.run([exe, '--set-gaps', '10'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertNotEqual(res.returncode, 0)

    def test_cross_dunst_corner_radius_and_wm_corner_radius(self):
        self.run_app('wm-gui', ['--set-corner-radius', '14'])
        self.run_app('notifications-gui', ['--set-margin', '10'])
        
        rc_path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        tree = ET.parse(rc_path)
        self.assertEqual(tree.getroot().find('.//theme/cornerRadius').text, '14')

    def test_cross_swaylock_color_nord_theme_sync(self):
        self.run_app('lockscreen-gui', ['--set-ring-color', '81a1c1'])
        with open(os.path.join(self.xdg_config_home, 'swaylock/config'), 'r') as f:
            self.assertIn('ring-color=81a1c1', f.read())

    def test_cross_parallel_execution(self):
        exe_wm = self.get_bin('wm-gui')
        exe_ln = self.get_bin('launcher-gui')
        
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = self.xdg_config_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        
        p1 = subprocess.Popen([exe_wm, '--set-gaps', '20'], env=env)
        p2 = subprocess.Popen([exe_ln, '--set-width', '900'], env=env)
        
        p1.wait(timeout=5)
        p2.wait(timeout=5)
        
        self.assertEqual(p1.returncode, 0)
        self.assertEqual(p2.returncode, 0)

    def test_cross_config_reload_trigger(self):
        self.run_app('wm-gui', ['--set-gaps', '8'])


class RealWorldTests(BaseE2ETest):
    # --- Tier 4: Real-World Workload Scenarios ---
    def test_realworld_initial_setup(self):
        shutil.rmtree(self.xdg_config_home)
        os.makedirs(self.xdg_config_home)
        
        self.run_app('wm-gui', ['--set-gaps', '8', '--set-corner-radius', '10', '--set-border-width', '2'])
        self.run_app('notifications-gui', ['--set-timeout', '10', '--set-width', '300'])
        self.run_app('launcher-gui', ['--set-width', '600', '--set-opacity', '90'])
        self.run_app('lockscreen-gui', ['--set-ring-color', '3b4252', '--set-lock-time', '600'])
        self.run_app('screenshot-gui', ['--set-save-dir', '/tmp/sc', '--set-delay', '0'])
        
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'labwc/rc.xml')))
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'labwc/themerc-override')))
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'dunst/dunstrc')))
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'rofi/config.rasi')))
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'swaylock/config')))
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'labwc/autostart')))
        self.assertTrue(os.path.exists(os.path.join(self.xdg_config_home, 'screenshot/config')))

    def test_realworld_rapid_update_stream(self):
        for g in range(5, 15):
            self.run_app('wm-gui', ['--set-gaps', str(g)])
            
        rc_path = os.path.join(self.xdg_config_home, 'labwc/rc.xml')
        tree = ET.parse(rc_path)
        self.assertEqual(tree.getroot().find('.//core/gap').text, '14')

    def test_realworld_corrupt_config_recovery(self):
        labwc_dir = os.path.join(self.xdg_config_home, 'labwc')
        os.makedirs(labwc_dir, exist_ok=True)
        with open(os.path.join(labwc_dir, 'rc.xml'), 'w') as f:
            f.write("<labwc_config><core><gap>invalid XML")
            
        dunst_dir = os.path.join(self.xdg_config_home, 'dunst')
        os.makedirs(dunst_dir, exist_ok=True)
        with open(os.path.join(dunst_dir, 'dunstrc'), 'w') as f:
            f.write("invalid=syntax=no=section")
            
        self.run_app('wm-gui', ['--set-gaps', '12'])
        self.run_app('notifications-gui', ['--set-timeout', '10'])
        
        tree = ET.parse(os.path.join(labwc_dir, 'rc.xml'))
        self.assertEqual(tree.getroot().find('.//core/gap').text, '12')
        
        config = configparser.ConfigParser()
        config.read(os.path.join(dunst_dir, 'dunstrc'))
        self.assertEqual(config.get('global', 'timeout'), '10')

    def test_realworld_theme_profile_deployment(self):
        self.run_app('wm-gui', ['--set-gaps', '8', '--set-corner-radius', '12', '--set-border-width', '2'])
        self.run_app('lockscreen-gui', ['--set-ring-color', '81a1c1'])
        
        tree = ET.parse(os.path.join(self.xdg_config_home, 'labwc/rc.xml'))
        self.assertEqual(tree.getroot().find('.//core/gap').text, '8')
        self.assertEqual(tree.getroot().find('.//theme/cornerRadius').text, '12')
        
        with open(os.path.join(self.xdg_config_home, 'labwc/themerc-override'), 'r') as f:
            self.assertIn('window.border.width: 2', f.read())
            
        with open(os.path.join(self.xdg_config_home, 'swaylock/config'), 'r') as f:
            self.assertIn('ring-color=81a1c1', f.read())

    def test_realworld_session_migration(self):
        new_home = os.path.join(self.xdg_config_home, 'migrated_home')
        os.makedirs(new_home)
        
        exe = self.get_bin('wm-gui')
        env = os.environ.copy()
        env['XDG_CONFIG_HOME'] = new_home
        env['QT_QPA_PLATFORM'] = 'offscreen'
        
        res = subprocess.run([exe, '--set-gaps', '15'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertEqual(res.returncode, 0)
        
        path = os.path.join(new_home, 'labwc/rc.xml')
        self.assertTrue(os.path.exists(path))
        tree = ET.parse(path)
        self.assertEqual(tree.getroot().find('.//core/gap').text, '15')


if __name__ == '__main__':
    unittest.main()
