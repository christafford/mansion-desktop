#!/usr/bin/env python3
"""Desktop-entry parsing tests. Fixtures are not application compatibility claims."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "world/tools/desktop-apps.py"
spec = importlib.util.spec_from_file_location("desktop_apps", HELPER)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def app(extra):
    _, GioUnix, GLib, _ = module.libraries()
    key = GLib.KeyFile()
    data = "[Desktop Entry]\nType=Application\nName=An App\n" + extra
    key.load_from_data(data, len(data), GLib.KeyFileFlags.NONE)
    return GioUnix.DesktopAppInfo.new_from_keyfile(key)


class DesktopApps(unittest.TestCase):
    def test_exec_expansion_preserves_arguments_and_never_runs_shell(self):
        item = app('Exec=/usr/bin/printf "two words" %c %i %% %F %u "$(touch nope)"\nIcon=test-icon\n')
        argv, cwd = module.command(item)
        self.assertEqual(argv, ['/usr/bin/printf', 'two words', 'An App', '--icon', 'test-icon', '%', '$(touch nope)'])
        self.assertEqual(cwd, '')

    def test_bad_field_code_and_missing_executable_are_rejected(self):
        for value in ['/usr/bin/printf %Z', '/usr/bin/printf embedded%F', '/usr/bin/printf broken%']:
            with self.subTest(value=value), self.assertRaises(ValueError):
                module.command(app('Exec=' + value + '\n'))
        # GIO itself rejects missing executables before a command can be prepared.
        with self.assertRaises(TypeError):
            app('Exec=/does/not/exist\n')

    def test_working_directory_is_preserved(self):
        with tempfile.TemporaryDirectory(prefix='elsewhere ') as temp:
            self.assertEqual(module.command(app('Exec=/usr/bin/true\nPath=' + temp + '\n'))[1], temp)

    def test_catalog_precedence_hidden_nodisplay_and_localization(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            user = base / 'user/applications'
            system = base / 'system/applications'
            user.mkdir(parents=True)
            system.mkdir(parents=True)
            template = '[Desktop Entry]\nType=Application\nName=System App\nExec=/usr/bin/true\n'
            (system / 'override.desktop').write_text(template)
            (user / 'override.desktop').write_text(template.replace('System App', 'User App'))
            (system / 'hidden.desktop').write_text(template)
            (user / 'hidden.desktop').write_text('[Desktop Entry]\nHidden=true\n')
            (user / 'nodisplay.desktop').write_text(template + 'NoDisplay=true\n')
            (user / 'missing.desktop').write_text(template + 'TryExec=/missing-program\n')
            (user / 'localized.desktop').write_text(template + 'Name[fr]=Application française\n')
            env = dict(os.environ, XDG_DATA_HOME=str(base / 'user'), XDG_DATA_DIRS=str(base / 'system'), LANGUAGE='fr', LANG='C.UTF-8')
            result = base / 'result.json'
            subprocess.run(['/usr/bin/python3', str(HELPER), 'catalog', '--output', str(result), '--cache', str(base / 'icons')], env=env, check=True)
            entries = {x['id']: x for x in json.loads(result.read_text())['entries']}
            self.assertEqual(entries['override.desktop']['name'], 'User App')
            self.assertEqual(entries['localized.desktop']['name'], 'Application française')
            for hidden in ['hidden.desktop', 'nodisplay.desktop', 'missing.desktop']:
                self.assertNotIn(hidden, entries)

    def test_launch_exec_environment_and_error(self):
        import socket
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            apps = base / 'applications'
            apps.mkdir()
            receiver = base / 'receiver.py'
            receiver.write_text('import json,os,sys\nprint("actual client diagnostic",file=sys.stderr)\nfrom pathlib import Path\nPath(sys.argv[1]).write_text(json.dumps({"args":sys.argv[2:],"env":dict(os.environ),"cwd":os.getcwd()}))\n')
            marker = base / 'result.json'
            (apps / 'fixture.desktop').write_text('[Desktop Entry]\nType=Application\nName=Fixture\nExec=/usr/bin/python3 "' + str(receiver) + '" "' + str(marker) + '" "two words" %c %U\nPath=' + str(base) + '\n')
            env = dict(os.environ, XDG_DATA_HOME=str(base), XDG_DATA_DIRS=str(base / 'empty'), DISPLAY=':test', WAYLAND_SOCKET='88', DBUS_SESSION_BUS_ADDRESS='host-bus', ELSEWHERE_TERMINAL_ARGV='["/usr/bin/false"]')
            error = base / 'error.json'
            with socket.socket(socket.AF_UNIX) as display:
                display.bind(str(base / 'display'))
                args = ['/usr/bin/python3', str(HELPER), 'launch', '--output', str(error), '--id', 'fixture.desktop', '--socket', str(base / 'display')]
                subprocess.run(args, env=env, check=True)
                result = json.loads(marker.read_text())
                self.assertEqual(result['args'], ['two words', 'Fixture'])
                self.assertEqual(result['cwd'], str(base))
                self.assertEqual(Path(str(error) + '.log').read_text().strip(), 'actual client diagnostic')
                self.assertEqual(result['env']['WAYLAND_DISPLAY'], str(base / 'display'))
                self.assertEqual(result['env']['DBUS_SESSION_BUS_ADDRESS'], 'disabled:')
                self.assertNotIn('DISPLAY', result['env'])
                self.assertNotIn('WAYLAND_SOCKET', result['env'])
                (apps / 'fixture.desktop').unlink()
                self.assertEqual(subprocess.run(args, env=env).returncode, 1)
                self.assertIn('removed', json.loads(error.read_text())['error'])

    def test_chromium_recipe_preserves_sandbox_and_separates_host_profile(self):
        with tempfile.TemporaryDirectory() as temp:
            argv = module.graphical_command(['/usr/bin/google-chrome-stable', 'https://example.test'], temp)
            self.assertIn('--ozone-platform=wayland', argv)
            self.assertIn('--disable-gpu', argv)
            self.assertIn('--new-window', argv)
            self.assertNotIn('--no-sandbox', argv)
            self.assertIn('https://example.test', argv)
            self.assertIn('--user-data-dir=' + temp + '/google-chrome-stable', argv)
            self.assertEqual(module.graphical_command(['/usr/bin/editor'], temp), ['/usr/bin/editor'])
            self.assertEqual(module.graphical_command(['/usr/bin/google-chrome-stable', 'https://example.test'], temp), argv)

    def test_terminal_wrapper_uses_clean_prompt_and_exact_argv(self):
        import socket
        item = app('Exec=/usr/bin/printf "two words"\nTerminal=true\n')
        with tempfile.TemporaryDirectory() as temp, socket.socket(socket.AF_UNIX) as display:
            path = str(Path(temp) / 'display')
            display.bind(path)
            with mock.patch.object(module, 'applications', return_value={'fixture.desktop': item}), \
                 mock.patch.dict(os.environ, {'PS1': '$(host_prompt_function)', 'PROMPT_COMMAND': 'host_prompt_function'}), \
                 mock.patch.object(module.os, 'execvpe', side_effect=RuntimeError('exec boundary')) as execute:
                with self.assertRaisesRegex(RuntimeError, 'exec boundary'):
                    module.launch('fixture.desktop', path)
                executable, argv, env = execute.call_args.args
                self.assertTrue(executable.endswith('/weston-terminal'))
                self.assertIn('--shell=' + str(HELPER), argv)
                self.assertEqual(json.loads(env['ELSEWHERE_TERMINAL_ARGV']), ['/usr/bin/printf', 'two words'])
                self.assertEqual(env['PS1'], '$ ')
                self.assertNotIn('PROMPT_COMMAND', env)
                self.assertTrue(Path(env['INPUTRC']).is_file())
                self.assertEqual(os.environ['PS1'], '$(host_prompt_function)')


if __name__ == '__main__':
    unittest.main()
