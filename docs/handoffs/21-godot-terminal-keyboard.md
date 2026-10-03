# Keyboard application mode — 2026-10-03

P21-T42 builds on `d7a590c` and the preserved overnight tree. Enter now activates
the real terminal in a flat view; Ctrl+Alt+Escape returns to the study. Shell
typing, editing, modifiers, repeat, interrupt and ordinary shell exit are working.
This is the keyboard interaction gate, not full desktop or human acceptance.

## Run

```sh
tools/run-godot.sh --audio-driver Dummy
```

Wait for the terminal on the monitor and press Enter. Type ordinary shell
commands. Escape, Tab, Home, WASD and M belong to the application while active.
Ctrl+Alt+Escape releases held keys and returns to world mode. Losing host focus
also returns to world; activation is explicit when returning. The camera stops
moving/looking in application mode. Type `exit` to close the shell and return to
the room. Relaunch still means restarting the study; the terminal titlebar's
pointer controls are not routed yet.

The tested client is installed Weston 15.0.1-3 with `/bin/sh` (Bash on this
machine), monospace size 16. Weston starts the shell in the user's home, not the
repository. Its inherited zsh-style prompt/PROMPT_COMMAND initially produced
garbled prompts and shell errors. The launcher now supplies a plain prompt,
repository-local shell history/configuration and an explicit inputrc disabling
unsupported bracketed-paste control sequences. No host configuration was edited.
`--terminal-demo` remains an output demonstration; omit it to get the shell.

## Boundaries

`application_mode.gd` owns activation, the flat TextureRect and host-input policy.
It uses the same current ImageTexture as the world monitor. The view preserves
native pixel size when it fits, otherwise shrinks with aspect ratio intact.
Canvas presentation bypasses 3D room lighting/tone mapping. The world screen's
T41 inverse Filmic material remains unchanged.

`keyboard_map.gd` explicitly maps physical US key positions to Linux evdev codes,
including right/left modifiers and editing/function/keypad keys. Godot numeric
key values are never forwarded as evdev codes. The seat supplies its existing US
XKB keymap. IME/composed text, layout selection, clipboard and broader language
support remain unimplemented. Godot echo events are ignored; the client repeats
using the Wayland seat's 25 Hz / 500 ms repeat information.

The runtime resolves `focus_keyboard(handle)` only for a live mapped xdg
toplevel. `0` clears focus; invalid requests preserve the current target.
`keyboard_focus_handle()` exposes current validity; `keyboard_key(code, pressed)`
requires a valid focused target. No global legacy seat is used by Godot.

`seat.cpp` owns XKB state and held keys, ignores duplicate transitions, and
delivers keys/modifiers only to keyboard resources of the focused client.
Focus changes release held keys, clear depressed/latched/locked modifiers, send
leave and then enter/initial modifiers. A late keyboard binding gets the current
pressed array and modifiers. A surface-destroy listener clears the borrowed
focus pointers before resource memory disappears, without sending leave with a
destroyed surface argument. The runtime also revokes focus on detach or role
loss after dispatch. Event serials come from the owning display; timestamps are
monotonic milliseconds. Shared legacy seat behavior retains its regression suite.

Protocol reference: [Wayland keyboard specification](https://wayland.freedesktop.org/docs/html/apa.html),
checked against installed `wayland.xml`; seat v4 uses client-side repeat.

## Verification

Observer: Codex agent, 2026-10-03, T42 working tree based on `d7a590c`, committed
with this handoff. Godot `4.7.2.stable.official.ed1daf0bf`, godot-cpp
`507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, Compatibility/OpenGL 4.6,
Mesa 26.2.4, AMD Custom GPU 0405, 1280×800 viewport.

```sh
meson compile -C build
meson test -C build --print-errorlogs
tools/check-godot-binding.sh
tools/check-godot-study.sh
```

- C++ build and 33/33 Meson tests pass, including legacy focus/input regressions.
- Existing binding, protocol lifecycle and owned-frame suites pass. The binding
  check now also runs `runtime-keyboard` against real socket-connected fixtures.
- The new protocol test uses two clients, two windows each and role-less surfaces.
  It asserts exact US Shift+A/lowercase key values, releases, modifier masks,
  duplicate suppression, no cross-client delivery, same-client/idempotent focus,
  late keyboard held state, Caps Lock reset, detach, focused surface destruction,
  invalid/stale handle rejection and cleanup.
- The keyboard runtime test passes AddressSanitizer, UBSan and enabled leak
  detection. This covers project runtime/seat code, not instrumented Godot.
- Import/runtime and the existing rendered controller/color/transform/live-output
  checks pass. A headless mapping test checks 27 key/location/fallback cases.
- `terminal_input.gd` injects real Godot input events through the production
  routing path. It types commands that write files from the actual shell; verifies
  mixed case/punctuation and Backspace, client repeat (nine x characters in the
  recorded run), Ctrl+C interruption of `sleep`, repeated activation of the same
  window, world movement/release, focus-loss notification and shell exit while
  Shift is held. The test does not inject compositor pixels or shell output.
- The flat GPU capture matches 7,571 sampled opaque source pixels within two
  bytes per channel, at native size. Actual command output and the return to world
  were also visually inspected. This does not prove latency or physical input.

One controller process exited without its success marker during a GUI run; the
runner rejected it. The isolated recheck and subsequent full study suite passed;
no script error was reported and
the cause of that early exit was not established. Success requires the full
markers and error-free logs, never just a zero process status.

Session logs are under `.tools/recovery/t42/`; individual check logs are retained
under `/tmp`. Generated captures go to `.tools/terminal-output-test/` and
`.tools/terminal-input-test/`, preserving committed historical captures.
[Selected evidence](../evidence/terminal-input/) accompanies this change.

To reproduce the sanitizer check, use the T40 handoff's sanitizer build command
with `tests/test_runtime_keyboard.cpp`, output `runtime-keyboard-asan`, and
fixture `build-godot-probe/keyboard-client`.

## Exact continuation

P21-T43: implement application-view pointer mapping/button/scroll. P21-T44 then
handles xdg resize/configure; P21-T45 handles close requests and explicit relaunch.
Reuse this seat, active handle and TextureRect geometry. Keep ordinary client
close distinct from launcher process shutdown; never kill a client to pretend
it accepted `xdg_toplevel.close`. Preserve all current regressions and perform
the real-client checks in [TASKS.md](../TASKS.md).

No broader or human task is ticked. Physical keyboard/host shortcuts, pointer,
resize, latency, final art and full usability still need their stated evidence.
Unrelated overnight source/binary changes remain outside this commit.
