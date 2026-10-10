# Application wheel and search — 2026-10-04

P21-T48 builds on `7316f78`, preserving the two intervening navigation commits
and all unrelated overnight edits. The A/D behavior already matched the owner's
request: turn by default, strafe with right mouse held. Codex rechecked it and
added the missing explicit D-strafe assertion and corrected the on-screen help.

## Use

Run `tools/run-godot.sh --audio-driver Dummy`. In the room, press Tab or click
Applications. Eight slots show distinct recent apps, newest at twelve o'clock
and then clockwise. Empty history slots stay empty. Click the center and type
an installed application's name, description or keywords; Enter, double-click
or Launch starts the selected result. Esc backs out of search and then the wheel.
Numbers 1–8 select occupied recent slots. Tab remains ordinary application input
when an application is focused. Ctrl+Alt+Escape returns to the room.

The startup Terminal becomes a real recent entry after its first mapped frame.
Selecting a live recent app restores it. After typing `exit` in Terminal, select
Terminal again to relaunch with a new window handle. History contains only up to
eight desktop-entry IDs in `.tools/launcher-state/recent.json`; successful relaunch
promotes/deduplicates its entry. No history is fabricated to fill the wheel.

## Implementation and limits

The requested MIT Advanced Radial Menu is vendored at
`decd8679efd19947a54bbe99fc5644846322f745`. Its sole local change allows a floating
rotation offset for exact top-centered sectors. License and hashes are retained
in its UPSTREAM.md. [Decision 07](../decisions/07-application-launcher.md) records
the scope and dependencies before adding them.

Godot owns the UI and input boundary. An asynchronous Python/GIO helper discovers
freedesktop applications with localization, Hidden/NoDisplay and directory
precedence; GTK resolves installed icon themes, including the optional KDE theme
preference. No Plasma/KRunner service is invoked or required. The current
container exposes 16 visible entries, including Elsewhere's entry for the installed
Weston terminal. This is not a claim to discover every application on the host.

Launch resolves a desktop ID again, expands Exec arguments without a shell,
honors Path and wraps Terminal=true entries in Weston. It replaces the helper
process with the application, retaining Godot's child ownership. Weston launches
use the same isolated config, simple prompt and inputrc as the initial terminal.
Client processes receive the private Wayland socket, no DISPLAY/WAYLAND_SOCKET,
Wayland toolkit preferences and a disabled host session-bus address. Flatpak
entries are explicitly unavailable. These environment settings are not a process
sandbox; application-specific single-instance IPC could still reuse a host
instance. Do not claim general host-instance isolation or general application
compatibility from the verified Terminal/Vim launches.

Only one launch is pending at a time. A newly mapped window becomes its initial
presentation candidate; this is a temporary association, not durable identity or
complete multiwindow attribution. History changes only after mapped content.
Failures/timeouts are shown without replacing the old monitor content. Child
processes remain owned for disconnect cleanup and the existing bounded shutdown
fallback; process termination is not used as an application-close request.

## Verification and corrections

Observer: Codex agent, 2026-10-04, based on `7316f78` plus this task's changes.
Godot `4.7.2.stable.official.ed1daf0bf`, Compatibility/OpenGL 4.6, Mesa 26.2.4,
AMD Custom GPU 0405, 1280×800. Installed Weston 15.0.1 and Vim; Python 3.14.7,
PyGObject 3.56.3, GIO/GioUnix 2.88.3 and GTK 3.24.52.

`tools/check-godot-study.sh` passes import/runtime, six Python tests and all ten
Godot test processes: transforms, keyboard/pointer mapping, navigation, color,
live terminal output, keyboard, pointer, resize and launcher. Logs are retained
locally under `.tools/recovery/t48/`. The final launcher check also runs separately
with isolated state after the last test refinements. No C++ implementation changed;
the validator rebuilt/staged the existing extension. Historical Meson/native
protocol/sanitizer results are not presented as fresh tests for this task.

The Python fixtures cover localized names, user-entry precedence, Hidden and
NoDisplay suppression, TryExec, quoted arguments, field expansion and rejection,
working directory, removed entries, private child environment and prompt isolation.
The graphical test clicks the actual wheel center, types search text through
Godot's GUI, checks no-results and cancel behavior, launches installed Vim,
types into its real terminal, clicks the recent Terminal slot, verifies that
application Tab is not intercepted, exits/relaunches Terminal and reads a
shell-written `LAUNCHER_OK` file. It verifies child cleanup and eight-entry MRU
ordering, deduplication, reload and exact clockwise layout. The eight-entry
history test is a fixture, not eight successful application compatibility trials.

[Rendered evidence](../evidence/application-launcher/) was inspected. Vim accepts
basic editing, but Weston logs unsupported terminal escape sequences; this is
not full Vim/terminal emulation acceptance. Weston graphical editor was tried
and failed with `No text input manager global`, so its provisional built-in entry
was removed rather than offered as a working editor. A first GUI test used
keycodes without Unicode text in the search field; that harness was corrected.
One relaunch trial missed its shell marker; a bounded settle now precedes typing
after initial mapping/activation resize. Inspection also caught inherited host prompt commands/
formatting; the production helper now isolates those settings, with a regression
test. One earlier D-strafe test failed transiently; the diagnostic rerun and final
full-suite run pass unchanged movement code. A later isolated launcher run lost
activation during relaunch; a rerun with richer host/window-focus diagnostics
passed. The triggers of those intermittent failures were not established; logs
retain them and the test now stops before typing when activation fails. No
production focus policy was bypassed or failing assertion removed.

No physical-input, latency or human comfort result is claimed. GTK missing-theme
warnings in the isolated metadata fixture and PyGObject's own deprecation warning
are distinct from failures; real installed icons resolved and were rendered.

## Next bounded task

**P21-T45**: implement targeted `xdg_toplevel.close` and the documented close
action. Reuse T48's launcher/relaunch and child ownership; do not build a second
launcher. Verify deferred/declined closes and multiple windows per client as
specified in TASKS.md. T45 remains open because shell exit/relaunch does not prove
compositor close requests. Preserve the outstanding cursor, clipboard, popup,
general graphical-client and multiwindow gates. Broad and human gates stay open.
