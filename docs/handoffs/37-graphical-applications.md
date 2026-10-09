# Real Chrome startup and subsurface composition

Owner request 2026-10-09, from `2cdfd15`, P21-T65 and its independently tested
surface-tree prerequisite P21-T66. This is bounded Chrome support through the
retained Wayland compositor, not general Linux application compatibility.

## Diagnosis and implementation

The launcher's old protocol warning was a generic twelve-second timeout. An
isolated Chrome 154.0.8037.57 on Mansion's socket repeatedly synchronized without
creating a toplevel: no `wl_output` was advertised. Adding a fixed logical output
let it map real pixels. Typing then crashed Chrome while creating the omnibox's
subsurface. A debugger reproduced the null proxy access, and Chromium's
[Wayland bubble implementation](https://chromium.googlesource.com/chromium/src/+/main/ui/ozone/platform/wayland/host/wayland_bubble.cc)
explains the missing `wl_subcompositor` operation. Implementing subsurfaces fixed
that reproduced crash. Pointer v5 frame events were also necessary for Chrome's
wheel delivery. Diagnostic logs/scripts remain ignored under `.tools/t65/`.

The owned runtime now provides:

- One `wl_output` v3: 1280×800 logical display, scale 1, normal transform, 60 Hz
  nominal mode. This is a nested workspace, independent of the host's monitors;
  it does not promise measured 60 Hz presentation. Bindings have version-correct
  metadata, release/destruction and per-client surface enter/leave tracking.
- `wl_subcompositor` v1 with copied buffers, cached synchronized commits and
  callbacks, inherited synchronization, desynchronized updates, parent-committed
  positions/stacking, immediate removal, and resource-safe parent/child teardown.
  Invalid roles, parents, cycles and sibling requests produce protocol errors.
- Owned composited toplevel snapshots with scale/transform conversion, straight
  alpha blending and negative child extents. Child windows remain part of their
  parent's application entity rather than becoming movable application panels.
  Canvas origins translate pointer coordinates; hit testing respects copied,
  double-buffered input regions, stacking and implicit child pointer grabs.
- Runtime seat v5 pointer frames and wheel source events, retaining v1/v4 client
  behavior. The legacy frontend continues to advertise v4, since it has a
  separate pointer delivery path.

Protocol semantics follow the [Wayland specification](https://wayland.freedesktop.org/docs/html/apa.html).
No protocol stubs, Xwayland bridge, GPU-buffer import, or dependency were added.
One toplevel's composite is cached by its visible tree signature. Opaque bottom
rows use direct copies; child alpha/transform work runs only when needed.
Retained core/raw/cached/composed buffers share a conservative 256 MiB allocation
budget; a composed canvas is limited to 4096 pixels per dimension / 64 MiB.
Hierarchies are limited to 64 levels and 255 direct children per parent;
subsurface offsets to ±8192; input regions to 1024 ordered rectangle operations.
These limits reject explicitly instead of overflowing, allocating without a bound
or silently clipping a child to the root rectangle. Caller-retained snapshots
remain outside the compositor's storage accounting, as before.

Chrome sometimes acknowledges a new size while first committing an old-sized
buffer. Effective xdg geometry is correctly clamped when applied and then fixed
until another geometry request. The frontend now uses separately recorded client-
declared geometry for decoration margins, so that temporary clamping cannot
produce a permanently excessive margin on the next host resize. Default geometry
follows independently resized child content; explicit geometry clamping includes
the applied subsurface bounds.

## Launcher behavior

Known native Chrome/Chromium executable names receive `--ozone-platform=wayland`,
`--disable-gpu`, `--new-window`, and a separate profile under
`.tools/browser-profiles/<binary>/`. Browser sandboxing stays enabled; neither
`--no-sandbox` nor a host desktop/service change is used. The profile keeps browser
state separate from the user's ordinary browser. Concurrent Mansion frontends
should use different `MANSION_BROWSER_PROFILE_DIR` roots; the real-browser test
always uses its own root. This override is also useful for a clean diagnostic
profile. The command recipe is tested for Chromium names, but only installed
Google Chrome is a verified browser here.

A second launch can exit after forwarding a new-window request to the first
owned browser process. The launcher now waits for its mapped window until the
startup deadline instead of treating that helper exit as immediate failure.
Other third-party single-instance IPC still has no general isolation solution.

Each exec'd client's stderr goes to the unique `launch-*.json.log` beside its
launch error JSON. The original PID/exec ownership model is retained. A timeout
now distinguishes an exited process from one still waiting, and shows the log
path rather than asserting a missing protocol. Pre-exec errors still show their
specific error. Logs and browser profiles are ignored generated data.

## Verification and scope

Agent-run evidence, 2026-10-09, `2cdfd15` plus this change, Google Chrome
154.0.8037.57, Godot 4.7.2 ed1daf0bf, Compatibility/OpenGL, Mesa 26.2.4, AMD Custom
GPU 0405, private GPU Weston. The HTML page and localhost HTTP server are test
content only; Chrome is the installed, unmodified real application. Godot input
injection produces browser-trusted keyboard/pointer events and the page reports
its typed value, click count, scroll and viewport dimensions over HTTP.

The browser check exercises catalog launch, omnibox typing and its child surface,
real page navigation, text entry, pointer click, wheel scrolling, actual resize
configure/ack/commit and DOM reflow, return to the room, a second browser window
with a distinct entity, and the browser's own Ctrl+Shift+W close of that window
while the first survives. It does not substitute process termination for a
window-close implementation. The native fixture separately covers synchronization,
stacking, regions, transformed/scaled partial-alpha pixels, grabbed coordinates,
callback timing, retained snapshots and malformed relationships.

Current validation passed:

- Meson compile and all **34 tests**: `.tools/t65/default-geometry-build.log` and
  `default-geometry-tests.log` (including independently resized child bounds).
- Standard native/Godot binding suite, including the tree fixture:
  `.tools/t65/binding.log`. Final equivalent core changes also passed Meson and
  the rebuilt graphical suites below.
- ASan + UBSan with leak detection: `final-sanitize.log` (15 tree stages and
  three invalid relationships), `runtime-sanitize.log` (16 lifecycle trials,
  including output bind/release/unmap/remap and server-stop cleanup).
- Full study suite: **20 Godot trials and 8 Python tests**,
  `.tools/t65/full-study.log`, `/tmp/mansion-study-check.ns8C80`. This preceded
  the final default-geometry fix; the affected terminal-resize trial passed again
  afterward (`final-terminal-resize.log`, four cell sizes, zero failures), along
  with the final native, tree sanitizer and real-browser checks above/below.
- Final real-browser check and import/runtime validation:
  `.tools/t65/browser-default-geometry.log`, requiring `CHROME_APPLICATION_OK failures=0`.
  The capture-return sequence uses repeated wheel events because Chrome bounds
  each large wheel event; one event was insufficient to scroll fully to the top.
- **62 plugin/parser tests**, shell syntax, Python syntax, numeric task IDs,
  relative documentation links and diffs.

Agent-inspected rendered captures: [address bar](../evidence/t65/address-bar.png),
[typed/clicked page](../evidence/t65/browser-input.png),
[resized page](../evidence/t65/browser-resized.png), and
[room return](../evidence/t65/browser-in-room.png). The source page, source frames
and Godot captures are distinguished; these linked images are actual engine
captures. No human acceptance is inferred from them.

Reproduce:

```sh
meson compile -C build
meson test -C build --print-errorlogs
SDL_JOYSTICK_LINUX_CLASSIC=1 tools/check-godot-binding.sh
tools/check-godot-study.sh
tools/check-chrome-application.sh
```

The last two commands need a working graphical display. The Chrome check also
needs the installed `google-chrome.desktop` entry; absence is a failure, not a
compatibility pass. The private Weston wrapper used here is
`.tools/t65/godot-check.sh` via `GODOT_BINARY`. The browser test uses an isolated
profile and local page, and does not visit personal accounts or require an
external website. The sanitizer driver is `.tools/t65/sanitize.sh`.

Remaining boundaries are explicit: `xdg_popup` menus, clipboard/primary selection,
client cursor images, IME/text-input protocols, D-Bus activation, Flatpak/host
handoff, hardware-accelerated application buffers and general GTK/Qt/Electron
compatibility are not established by this trial. In particular, an `xdg_popup`
request still disconnects its client. Browser menus requiring it are not usable.
The broader T45 compositor-requested close gate remains open. Human physical
input, browser usability and latency/comfort: **not observed**.
