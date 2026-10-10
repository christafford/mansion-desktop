# Handoff 04-recovery — P4-T06 Reconciliation (2026-09-29)

Purpose: reconcile implementation against documentation and establish a
baseline for the foundation recovery tasks (P4-T06–P4-T18).

Commands used in this reconciliation:

```sh
meson compile -C build
meson test -C build --print-errorlogs
meson test -C build-asan --print-errorlogs   # first run since build was last
node --test .opencode/tests/*.test.js
```

---

## Baseline results (2026-09-29, this session)

| Check | Command | Result |
| --- | --- | --- |
| Normal build | `meson compile -C build` | Up to date (no rebuild needed) |
| Normal tests | `meson test -C build --print-errorlogs` | **25/25 pass, 0 fail** |
| ASan build | `meson setup build-asan` + `meson compile -C build-asan` | Built 8 targets; warnings present (see below) |
| ASan tests | `meson test -C build-asan --print-errorlogs` | **25/25 pass, 0 fail, no sanitizer errors** |
| Plugin tests | `node --test .opencode/tests/*.test.js` | **49/49 pass** |

Environment: Arch Linux container `elsewhere-dev`, gcc 16.2.1, meson 1.12.1,
ninja 1.13.2, mesa 26.2, wayland 1.26, wayland-protocols 1.49, node 26.

---

## Contradictions: documentation vs source

### C1. Unused X11 dependency in meson.build

`meson.build` line 13 declares `x11 = dependency('x11')` but:
- No source file in `src/` includes any X11 header (`grep -rn "x11" src/` → 0 matches)
- No target links against the x11 library (only reference in meson.build is line 13)
- X11 was fully removed in commits d4f8551–a615f18

**Impact:** The x11 dependency is dead code in meson.build. It is harmless
(because nothing links it) but should be cleaned up.

### C2. P1-T07 evdev claim contradicts source

STATUS.md line 228-231 states:
> "At the time, input_init()/input_destroy()/input_process() were no-ops and
> X11 events drove windowed mode; grep -r "/dev/input" src found nothing."

Reality in `src/input.cpp` lines 130–188:
- `input_init()` scans `/dev/input/` for keyboard and pointer evdev devices
- `input_destroy()` closes the evdev file descriptors
- Headless mode still opens no devices (early return on `opendir("/dev/input")`)

The X11 path was removed but evdev reading was added back in the same
migration. The historical claim about "no /dev/input" is correct for the
pre-migration state but contradicts current source.

### C3. P5-T01/P5-T02 "historical" tick marks vs revised acceptance

TASKS.md and STATUS.md record historical P5-T01/P5-T02 completion (bb8cd9d)
with a synthetic `focus-cycle` test. The revised P5-T01/P5-T02 acceptance
is materially stricter and has not been demonstrated:
- Existing test: two synthetic clients, tab/shift_tab in World mode, compositor
  does not crash
- Revised P5-T01 acceptance: multiple clients, multiple windows from one
  client, unmapped windows, legal teardown, abrupt disconnect, sanitizer pass
- Revised P5-T02 acceptance: two same-client windows cycle correctly; world
  selection emits no app typing; focus loss/exit releases held input;
  destroyed candidates are skipped

The partial registry code exists (`toplevel_list`, `toplevel_count`,
`toplevel_link`, `xdg_shell_cycle_focus()`) but has not satisfied the revised
acceptance criteria.

---

## Partial implementations

### P5-T01 window registry (partial)

| Aspect | Status |
| --- | --- |
| `toplevel_list` / `toplevel_count` in `ElsewhereCompositor` | Implemented (compositor.cpp:298-299, compositor-private.h:62-63) |
| `toplevel_link` in `ElsewhereXdgSurface` | Implemented (xdg-shell.cpp:37) |
| `toplevel_register()` / `toplevel_unregister()` | Implemented, with idempotency check for surface_resource |
| Registration on xdg_toplevel creation | Implemented (xdg-shell.cpp:202) |
| Unregistration on xdg_surface destroy | Implemented (xdg-shell.cpp:164) |
| Unregistration on wl_surface destroy | Implemented (xdg-shell.cpp:174) |
| `xdg_shell_cycle_focus()` | Implemented (xdg-shell.cpp:371-420) |
| Input script `tab` / `shift_tab` commands | Implemented (input.cpp:765-774), World mode only |
| Test `focus-cycle` | Implemented and passing |
| **Revised acceptance** | **NOT MET** — no multiple-window, unmapped, teardown-order, or sanitizer regression |

### P5-T02 focus cycling (partial)

| Aspect | Status |
| --- | --- |
| World-mode-only cycling | Implemented (input.cpp:766-774 gate on InputMode::World) |
| Forward / backward via tab / shift_tab | Implemented |
| Uses `toplevel_list` from P5-T01 | Yes |
| **Revised acceptance** | **NOT MET** — no held-input release, no same-client windows, no world-mode isolation verification |

### Native Wayland host window

| Aspect | Status |
| --- | --- |
| `wl_egl_window` client window | Implemented in display.cpp (d4f8551–a615f18) |
| Seat + relative-pointer listeners | Implemented (display.cpp, input.cpp) |
| Mesa Wayland EGL backend path | Implemented (eglGetPlatformDisplay with wl_client_display) |
| Flicker + no host input | **UNVERIFIED** — handoff 04-host-window documents but no runtime observation |
| Root cause hypothesis | Plausible (Mesa internal wl_display conflict) — unverified |
| Headless path | Unaffected, all headless tests pass |

### Orphaned surface list

| Aspect | Status |
| --- | --- |
| Surfaces retained after client disconnect | Implemented (compositor.cpp:30-48, 333-334) |
| Rendered oldest-first in render path | Implemented (display.cpp:1002-1205) |
| Cleanup on compositor shutdown | Implemented (compositor.cpp:333-334, in cleanup_compositor) |
| **Bounded retention** | **NOT IMPLEMENTED** — no size limit or age-based eviction; surfaces persist until shutdown |

---

## Known issues (not contradictions, just observations)

### K1. xdg-shell is minimal

`xdg_wm_base` v3 with `xdg_surface`/`xdg_toplevel` only. One fixed 800x600
configure, `ack_configure` serial stored but not checked,
`set_window_geometry`/title/app_id/min/max ignored, `xdg_positioner` accepted
and ignored, `get_popup` posts a protocol error, no `ping` sent.

### K2. ASan build warnings

The ASan build produces warnings (not errors):
- `input.cpp:218` — unused function `send_pointer_axis`
- `input.cpp:140` — potential `snprintf` truncation for `/dev/input` path
- `display.cpp` — several unused `data` parameters and missing field initializers

These are code-quality issues, not runtime failures.

### K3. P2-T05 reopened

P2-T05 (accelerated client experiment) was historically ticked with a
PBuffer+shm fallback. The task is now reopened for direct GPU buffer import.
The fallback test (`render-egl`) passes but cannot satisfy the revised
acceptance. P4-T13 owns the renewed investigation.

### K4. No real-client evidence

All 25 automated tests use the synthetic `elsewhere-test-client`. No real
Wayland application (terminal, editor, etc.) has been observed running inside
Elsewhere. P1-T10, P4-T04, P4-T17 are human tasks — autonomous sessions do not
perform them.

### K5. `--commit-color` flag on test client

The test client supports `--commit-color RRGGBB` for live-update testing
(P2-T03). This is a useful fixture for color-change verification.

### K6. `render-egl` test is separate from P2-T05

The `render-egl` test (P2-T05 fallback) uses EGL PBuffer + `glReadPixels`
+ shm export. It passes. However, `EGL_WL_bind_wayland_display` symbols are
NULL on both X11 and `EGL_PLATFORM_WAYLAND_KHR` displays — no direct import
path.

---

## Automation verification status

| Area | Status | Notes |
| --- | --- | --- |
| Test client fixtures | OK | `elsewhere-test-client` connects, maps, disconnects cleanly |
| Shell test harness (`lib.sh`) | OK | 25 shell tests all pass in both `build` and `build-asan` |
| Plugin test suite | OK | 49 JS tests pass — scope parsing, AUTOCONTINUE_DONE, limits, persistence |
| Task parser | Partial | Parses P1–P4 numeric IDs; P5 numeric tasks visible; P4 recovery tasks visible |
| ASan gate | OK | 25/25 pass, no sanitizer errors |

**Open automation gaps for P4-T07:**
- Plugin assumes Projects 1–4 have open tasks; needs stable fixtures when all are done
- No explicit test for Project 5 task parsing visibility in generated instructions
- No test for reopened gates blocking progress
- No test for unknown scope rejection in normal runs

---

## Audit leads for follow-up tasks

| Lead | Target task(s) |
| --- | --- |
| Dead `x11` dependency in meson.build | P4-T06 cleanup or P4-T07 |
| Orphaned surface unbounded retention | P4-T08 lifetime audit |
| Partial P5-T01 registry (revised acceptance) | P5-T01 (blocked on P4-T28) |
| Partial P5-T02 focus cycling (revised acceptance) | P5-T02 (blocked on P5-T01) |
| Flickering + no host input | P4-T12 (bounded prototype) |
| P2-T05 direct GPU import | P4-T13 (reinvestigation) |
| ASan warnings (unused functions, snprintf truncation) | P4-T08 cleanup |
| xdg-shell minimal compliance | P4-T11 (handshake + metadata) |
| Focus-cycle test only covers synthetic crash guard | P5-T01/P5-T02 (new tests) |
| Plugin test assumptions about task lists | P4-T07 (fixture replacement) |

---

## Bounded next task

The next eligible task is **P4-T07** (Repair automation verification),
whose prerequisite P4-T06 is now satisfied. P4-T07 is bounded to plugin/test
changes only and does not touch compositor code.

P4-T08 (Audit resource and lifetimes) is eligible after P4-T06 and would
benefit from P4-T07's stable fixtures.

P4-T12 (Native Wayland host experiment) and P4-T13 (Accelerated buffer
import) are also eligible after P4-T06 but require hardware/observation
that may not be available in the container.

---

## Limitations of this reconciliation

- No real-application terminal trial was performed (P1-T10, P4-T04, P4-T17
  are human tasks; no terminal installed in container)
- No `WAYLAND_DEBUG=1` logs captured for the host window path
- No native Wayland host compositor observation (KWin, sway, etc.)
- No visual pixel evidence collected
- `meson build` is normal debug; only `build-asan` for sanitizer coverage
- The ASan build regenerates from scratch each time (no incremental rebuild)
- Plugin tests are self-contained; they do not exercise the live OpenCode
  server or an actual LLM provider

This reconciliation is a documentation/source audit and baseline verification.
It does not replace runtime observation of real applications.

---

## Evidence table (at time of reconciliation)

| Item | Evidence source | Claim | Source says | Status |
| --- | --- | --- | --- | --- |
| P1-T01 Headless harness | STATUS.md, `smoke-headless` test | Headless starts, socket created/removed, exits 0 | Source: `main.cpp` headless stub | Verified (25/25 tests) |
| P1-T02 Test client globals | STATUS.md, `client-globals` test | wl_compositor, wl_shm, wl_seat, xdg_wm_base advertised | Source: `compositor.cpp`, `xdg-shell.cpp` | Verified |
| P1-T03 xdg-shell replaces wl_shell | STATUS.md, `client-toplevel` test | xdg_surface+xdg_toplevel, configure 800x600 | Source: `xdg-shell.cpp` | Verified |
| P1-T04 wl_shm buffers + frame callbacks | STATUS.md, `client-frame` test | shm buffer attach/commit/release, frame done | Source: `compositor.cpp` | Verified |
| P1-T05 Offscreen rendering | STATUS.md, `render-shm` test | EGL pbuffer, shm→texture, screenshot PPM | Source: `display.cpp` render path | Verified |
| P1-T06 Seat objects | STATUS.md, `input-routing` test | keymap, repeat_info, focus, enter/leave | Source: `input.cpp` seat | Verified |
| P1-T06-A/B/C/D/E/F | STATUS.md | Multi-seat, modifiers, focus, pointer, input script, report-input | Source: `input.cpp` | Verified |
| **P1-T07 Host window input replaces evdev** | **STATUS.md line 228** | **"input_init() no-ops, /dev/input gone"** | **input.cpp:130-188 opens /dev/input** | **CONTRADICTION (C2)** |
| P1-T08 Launcher child lifecycle | STATUS.md, `launch-client` test | SIGCHLD reaping, client connect/disconnect logging | Source: `main.cpp`, `launch.cpp` | Verified |
| P1-T09 Lifecycle robustness | STATUS.md, `lifecycle` test | 3 connect/disconnect cycles + fresh client | Source: `compositor.cpp` orphaned list | Verified |
| P2-T01 Math module | STATUS.md, `math` test | vec3, mat4, perspective, look_at | Source: `math.h`, `test_math.cpp` | Verified |
| P2-T02 Perspective panel | STATUS.md, `render-panel` test | MVP projection, panel at monitor frame | Source: `display.cpp` room mode | Verified |
| P2-T03 Live updates | STATUS.md, `render-update` test | Two color commits, red→blue | Source: `needs_upload` flag | Verified |
| P2-T04 Texture lifetime | STATUS.md, `render-lifecycle` test | 3 cycles, no GL errors | Source: texture creation/deletion | Verified |
| P2-T05 Fallback experiment | STATUS.md, `render-egl` test | EGL PBuffer→shm→compositor upload | Source: test client `--egl` flag | Verified (fallback only; task reopened) |
| P2-T07 Frame timing | STATUS.md | `--stats` flag, per-frame averages | Source: `render()` timing | Historical report |
| P3-T01 Explicit modes | STATUS.md, `mode-routing` test | World vs Application input separation | Source: `enum class InputMode` | Verified |
| P3-T02 Reserved shortcut | STATUS.md, `mode-exit` test | F12 exits, held keys released, leave events | Source: `exit_application_mode()` | Verified |
| P3-T03 Host focus loss | STATUS.md, `focus-loss` test | Stored mode, focus gained re-enters | Source: `stored_mode_when_focus_lost` | Verified |
| P3-T04 Full-size presentation | STATUS.md, `present-fullsize` test | Fullscreen 2D render, resize configure | Source: `render_application_fullscreen()` | Verified |
| P3-T05 Targeting and selection | STATUS.md, `select-panel` test | Panel targeted, Enter enters app mode | Source: `panel_targeted` | Verified |
| P3-T06 Client exit during app mode | STATUS.md, `app-exit` test | Return to world, clear focus, no dangling ptr | Source: `seat_clear_focus_state()` | Verified |
| P4-T01 Room geometry/collision | STATUS.md, `room-render`+`room-collision` | AABB clamp, flat-shaded room meshes | Source: `room.cpp`, `display.cpp` | Verified |
| P4-T02 Monitor slot/teleport | STATUS.md, `teleport` test | T key teleports, panel on monitor frame | Source: `input_teleport_key_set` | Verified |
| P4-T03 Milestone 1 scripted demo | STATUS.md, `milestone1` test | 9-step journey: walk, select, type, return | Source: `tests/milestone1.sh` | Verified |
| **P5-T01 Window registry** | **STATUS.md, `focus-cycle` test** | **Partial code, synthetic only** | **No multiple-window/unmapped/teardown test** | **NOT MET (revised acceptance)** |
| **P5-T02 Focus cycling** | **STATUS.md, `focus-cycle` test** | **Partial code, synthetic only** | **No same-client/held-input/escape test** | **NOT MET (revised acceptance)** |

Notes on the table:
- "Verified" means the listed test passes and the source code supports the
  described behavior. This is automated evidence, not real-application evidence.
- "CONTRADICTION" means documentation claims something that source does not support.
- "NOT MET" means partial implementation exists but revised acceptance criteria
  have not been satisfied.
