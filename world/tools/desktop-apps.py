#!/usr/bin/python3
"""Desktop-entry discovery/icons and direct child exec for the nested desktop."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys


def libraries():
    import gi
    gi.require_version("Gtk", "3.0")
    gi.require_version("GioUnix", "2.0")
    from gi.repository import Gio, GioUnix, GLib, Gtk
    return Gio, GioUnix, GLib, Gtk


def write_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False))
    temporary.replace(path)


def applications():
    Gio, GioUnix, _, _ = libraries()
    apps = {app.get_id(): app for app in Gio.AppInfo.get_all()
            if isinstance(app, GioUnix.DesktopAppInfo) and app.should_show()}
    # Weston ships its clients without desktop entries on this development host.
    for identifier, name, binary, icon in [
        ("mansion-weston-terminal.desktop", "Terminal", "weston-terminal", "utilities-terminal"),
    ]:
        executable = shutil.which(binary)
        if executable:
            _, _, GLib, _ = libraries()
            key = GLib.KeyFile()
            data = f"[Desktop Entry]\nType=Application\nName={name}\nExec={executable}\nIcon={icon}\n"
            key.load_from_data(data, len(data), GLib.KeyFileFlags.NONE)
            apps[identifier] = GioUnix.DesktopAppInfo.new_from_keyfile(key)
    return apps


def command(app):
    """No file/URI arguments are supplied by this application-only launcher."""
    _, _, GLib, _ = libraries()
    raw = app.get_string("Exec") or ""
    ok, words = GLib.shell_parse_argv(raw)
    if not ok or not words:
        raise ValueError("This entry has no executable command")
    if "%" in words[0]:
        raise ValueError("Field codes in the executable are unsupported")
    result = []
    for word in words:
        if word in ("%f", "%F", "%u", "%U", "%d", "%D", "%n", "%N", "%v", "%m"):
            continue
        if word == "%i":
            if app.get_string("Icon"):
                result.extend(["--icon", app.get_string("Icon")])
            continue
        def expand(match):
            code = match.group(1)
            if code == "%": return "%"
            if code == "c": return app.get_name()
            if code == "k": return app.get_filename() or ""
            raise ValueError(f"Unsupported or embedded Exec field code %{code}")
        result.append(re.sub(r"%(.)", expand, word))
        if word.endswith("%") and not word.endswith("%%"):
            raise ValueError("Incomplete Exec field code")
    if not shutil.which(result[0]):
        raise ValueError("Executable is unavailable: " + result[0])
    if Path(result[0]).name in ("flatpak", "distrobox-host-exec", "host-spawn"):
        raise ValueError("Host/sandbox application launching is not supported yet")
    directory = app.get_string("Path") or ""
    if directory and not Path(directory).is_dir():
        raise ValueError("Application working directory is unavailable")
    if app.get_boolean("Terminal") and not shutil.which("weston-terminal"):
        raise ValueError("Weston terminal is unavailable")
    return result, directory


def icon_theme():
    _, _, _, Gtk = libraries()
    name = "breeze"
    config = configparser.ConfigParser(interpolation=None, strict=False)
    try:
        config.read(Path(os.environ.get("XDG_CONFIG_HOME", str(Path.home() / ".config"))) / "kdeglobals")
        name = config.get("Icons", "Theme", fallback=name)
    except (configparser.Error, OSError):
        pass
    theme = Gtk.IconTheme.new()
    theme.set_custom_theme(name)
    return theme


def catalog(cache):
    _, _, _, Gtk = libraries()
    theme = icon_theme()
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    entries = []
    for identifier, app in applications().items():
        reason = ""
        try:
            command(app)
        except Exception as error:
            reason = str(error)
        icon_path = ""
        try:
            info = theme.lookup_by_gicon(app.get_icon(), 64, Gtk.IconLookupFlags.FORCE_SIZE) if app.get_icon() else None
            if info is None:
                info = theme.lookup_icon("application-x-executable", 64, Gtk.IconLookupFlags.FORCE_SIZE)
            if info:
                path = cache / (hashlib.sha256(identifier.encode()).hexdigest() + ".png")
                info.load_icon().savev(str(path), "png", [], [])
                icon_path = str(path)
        except Exception:
            # Missing/broken third-party icons retain the frontend's text label.
            pass
        entries.append({"id": identifier, "name": app.get_name(),
                        "description": app.get_description() or app.get_generic_name() or "",
                        "keywords": list(app.get_keywords() or []), "icon": icon_path,
                        "unavailable": reason})
    return {"entries": sorted(entries, key=lambda entry: entry["name"].casefold())}


def graphical_command(argv, profile_root):
    # Chromium otherwise selects X11 or forwards to the host browser's process.
    # Keep a separate persistent profile, and request CPU-rendered wl_shm frames.
    binary = Path(argv[0]).name
    if binary not in ("google-chrome", "google-chrome-stable", "chromium", "chromium-browser"):
        return argv
    profile = Path(profile_root) / binary
    profile.mkdir(parents=True, exist_ok=True, mode=0o700)
    return argv + ["--ozone-platform=wayland", "--disable-gpu",
                   "--user-data-dir=" + str(profile.resolve()), "--new-window",
                   "--no-first-run", "--no-default-browser-check"]


def launch(identifier, socket):
    app = applications().get(identifier)
    if app is None:
        raise ValueError("Application was removed or is no longer visible")
    argv, directory = command(app)
    if not Path(socket).is_socket():
        raise ValueError("The Mansion Wayland display is unavailable")
    env = dict(os.environ)
    for key in ("DISPLAY", "WAYLAND_SOCKET", "WAYLAND_DEBUG", "ENV", "BASH_ENV", "PROMPT_COMMAND", "DESKTOP_STARTUP_ID"):
        env.pop(key, None)
    env.update(WAYLAND_DISPLAY=socket, GDK_BACKEND="wayland", QT_QPA_PLATFORM="wayland",
               SDL_VIDEODRIVER="wayland", ELECTRON_OZONE_PLATFORM_HINT="wayland",
               DBUS_SESSION_BUS_ADDRESS="disabled:", XDG_CURRENT_DESKTOP="Mansion",
               XDG_SESSION_TYPE="wayland")
    if identifier == "mansion-weston-terminal.desktop" or app.get_boolean("Terminal"):
        # Match the initial study terminal: host shell prompts/config must not
        # turn into escape garbage or execute unavailable prompt functions.
        config = Path(__file__).resolve().parents[2] / ".tools/terminal-config"
        config.mkdir(parents=True, exist_ok=True)
        env.update(XDG_CONFIG_HOME=str(config), HISTFILE=str(config / "history"),
                   PS1="$ ", PS2="> ",
                   INPUTRC=str(Path(__file__).resolve().parents[1] / "config/terminal.inputrc"))
    if identifier == "mansion-weston-terminal.desktop":
        argv += ["--font=monospace", "--font-size=16", "--shell=/bin/sh"]
    if app.get_boolean("Terminal"):
        env["MANSION_TERMINAL_ARGV"] = json.dumps(argv)
        argv = [shutil.which("weston-terminal"), "--font=monospace", "--font-size=16", "--shell=" + str(Path(__file__).resolve())]
    argv = graphical_command(argv, os.environ.get("MANSION_BROWSER_PROFILE_DIR") or Path(__file__).resolve().parents[2] / ".tools/browser-profiles")
    if directory:
        os.chdir(directory)
    os.execvpe(argv[0], argv, env)


def main():
    shell_command = os.environ.pop("MANSION_TERMINAL_ARGV", None)
    if shell_command and len(sys.argv) == 1:
        argv = json.loads(shell_command)
        os.execvp(argv[0], argv)
    parser = argparse.ArgumentParser()
    parser.add_argument("action", choices=["catalog", "launch"])
    parser.add_argument("--output", required=True)
    parser.add_argument("--cache")
    parser.add_argument("--id")
    parser.add_argument("--socket")
    args = parser.parse_args()
    try:
        if args.action == "catalog":
            write_json(args.output, catalog(args.cache))
        else:
            # Preserve exec/PID ownership while retaining the actual child's
            # diagnostics, including errors after Python has been replaced.
            log = Path(args.output + ".log")
            log.parent.mkdir(parents=True, exist_ok=True)
            fd = os.open(log, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
            os.dup2(fd, 2, inheritable=True)
            if fd != 2:
                os.close(fd)
            launch(args.id, args.socket)
    except Exception as error:
        write_json(args.output, {"error": str(error)})
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
