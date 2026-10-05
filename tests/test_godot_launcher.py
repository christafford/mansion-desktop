"""Launcher environment/argv regression; fixture engine does not prove GUI startup."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class GodotLauncherTest(unittest.TestCase):
    def test_joystick_default_override_and_argument_boundary(self):
        with tempfile.TemporaryDirectory(prefix="mansion launch ") as directory:
            root = Path(directory)
            scripts = root / "tools"
            scripts.mkdir()
            source = Path(__file__).resolve().parents[1] / "tools/run-godot.sh"
            shutil.copyfile(source, scripts / "run-godot.sh")
            build = scripts / "build-godot-runtime.sh"
            build.write_text("#!/bin/sh\nexit 0\n")
            build.chmod(0o755)
            engine = root / "fixture engine"
            engine.write_text(
                "#!/usr/bin/python3\nimport json, os, sys\n"
                "print(json.dumps([os.environ.get('SDL_JOYSTICK_LINUX_CLASSIC'), sys.argv[1:]]))\n"
            )
            engine.chmod(0o755)
            for override, expected in [(None, "1"), ("0", "0"), ("1", "1")]:
                with self.subTest(override=override):
                    env = dict(os.environ, GODOT_BINARY=str(engine))
                    env.pop("SDL_JOYSTICK_LINUX_CLASSIC", None)
                    if override is not None:
                        env["SDL_JOYSTICK_LINUX_CLASSIC"] = override
                    result = subprocess.run(
                        ["bash", str(scripts / "run-godot.sh"), "--audio-driver", "Dummy",
                         "--", "argument with spaces"],
                        env=env, text=True, capture_output=True, check=True,
                    )
                    mode, args = json.loads(result.stdout)
                    self.assertEqual(mode, expected)
                    self.assertEqual(args, ["--path", str(scripts / "../world"),
                                            "--audio-driver", "Dummy", "--", "argument with spaces"])


if __name__ == "__main__":
    unittest.main()
