# Godot startup crash mitigation — 2026-10-05

P21-T50, based on `d40dde5` plus the existing uncommitted work. The owner reported
an intermittent SIGSEGV immediately after the Godot banner, before the OpenGL
renderer identification. The earlier `bssh` GTK warnings came from a separate
successful engine run. No application-panel or compositor code was changed here.

## Diagnosis

The engine remains the pinned local `4.7.2.stable.official.ed1daf0bf` with SHA-256
`8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e`.
It is stripped. Disassembly, the existing local engine/SDL source, and a GDB
breakpoint locate these supplied crash addresses:

| Address | Path |
| --- | --- |
| `0x19d8ca8` | SDL integer parsing, dereferencing the numeric input string |
| `0x19ea307` | Integer conversion called by the joystick comparator |
| `0x1a003d5` | `sort_entries`, evdev branch after parsing `d_name + 5` |
| `0x4b84aeb` | Sort implementation calling that comparator |
| `0x1a08e80` | `LINUX_ScanInputDevices`, after sorting `/dev/input` entries |

GDB at `0x1a00360` prints real `event9` / `event5` directory names. Its caller
chain includes exactly the reported `0x1a08e80`, `0x1a12a54`, `0x1a12b48`,
`0x19af85b`, `0x4d5f1a`, `0x4d4c632` and `0x4d5dcc6`. This is SDL joystick
initialization before Godot creates its display server, not a room-script stack.
Local source: `tools/godot-4.7.2-stable/thirdparty/sdl/joystick/linux/SDL_sysjoystick.c`,
`sort_entries` and `LINUX_ScanInputDevices`; initialization order is in
`tools/godot-4.7.2-stable/main/main.cpp`.

The evdev comparator queries `/sys/class/input/eventN/device` during each
comparison to derive joystick ordering. Changing device ordering is a plausible
trigger, but the exact corruption/race was **not reproduced**. A normal retry
and a GDB run succeeded without mitigation, consistent with an intermittent
failure. The host's stored owner core was listed by `coredumpctl`, but reading
it was denied; no permissions were changed. Do not claim an engine root-cause
repair or universal startup guarantee from the following workaround.

## Bounded launcher change

`tools/run-godot.sh` defaults `SDL_JOYSTICK_LINUX_CLASSIC=1` immediately before
executing Godot, preserving an explicit nonempty caller override. This selects
SDL's classic `/dev/input/js*` joystick enumeration. Its comparator sorts the
numeric joystick index directly and avoids the evdev/sysfs-ordering branch.
GDB branch counters on this binary confirm **event=0, classic=2**, with successful
exit, versus the captured original evdev path.

The hint is documented in the local SDL header and the
[SDL reference](https://wiki.libsdl.org/SDL3/SDL_HINT_JOYSTICK_LINUX_CLASSIC).
It affects joystick handling, not Godot's host keyboard/mouse routing. Gamepad
behavior is not accepted by this task; classic mappings/features may differ.
The environment is scoped to the launched process tree, not the user's shell
configuration or host services. Direct Godot invocations bypass the launcher
default. An explicit `SDL_JOYSTICK_LINUX_CLASSIC=0` restores the original backend
for diagnosis; it also restores the affected path.

## Verification

- `python3 tests/test_godot_launcher.py`: default, explicit 0/1 and argument/path
  spaces pass against a fixture engine. Added to the study check entry point.
- `bash -n tools/run-godot.sh`: passes.
- Five separate `tools/run-godot.sh --audio-driver Dummy --quit-after 90
  --max-fps 60` processes render through Compatibility/OpenGL and exit 0.
- The existing `world/tests/application_objects.gd` runs through the updated
  launcher: real Terminal/Vim, changing previews, targeted typing, movement,
  focus/drag cancellation, close cleanup and shutdown pass.
- A launcher `--headless --audio-driver Dummy --quit-after 30 -- --no-terminal`
  process exits successfully. Native extension builds/stages through the normal
  launcher on every run. No C++ source or dependencies changed.

Fresh logs are under `.tools/t50-startup/`: `sort-stack.log`, `classic-sort.log`,
`launch-1.log` through `launch-5.log`, `objects.log`, and `headless.log`.
GDB command files there retain the exact breakpoint probes. Startup/interaction
checks are agent-run; physical keyboard/gamepad behavior is not inferred.
The pre-existing navigation assertion and portrait-host resize limit recorded
in handoff 26 remain; this task does not claim a fresh green full study suite.

The user's earlier `DRI_PRIME=1` message reports fallback to the only GPU (0),
and missing drag cursors were already present in passing terminal tests. Neither
matches this pre-render joystick stack. `bssh` emitted invalid-GDK-seat warnings;
its compatibility remains unverified. The later display-reset message alone
does not establish why that first session ended. These messages were not
suppressed or relabeled as successful application support.

## Next

Normal launch remains `tools/run-godot.sh --audio-driver Dummy`; no new user
flags are required. P21-T45 remains the next separate planned feature. Broader
GTK-client support, full navigation-suite reconciliation and a permanent upstream
SDL fix remain outside this bounded startup mitigation.
