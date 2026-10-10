# Project rename to elsewhere

Owner request 2026-10-09, P21-T67, from `9c2de0c`. The repository is now
[christafford/elsewhere](https://github.com/christafford/elsewhere), with the
local checkout at `/home/deck/code/elsewhere`.

## Scope

The rename covers maintained text and filenames: documentation, C/C++ types,
namespaces and header guards, Meson/CMake targets, native entry points, Godot
classes and addon/resource paths, test fixtures, shell helpers, temporary socket
prefixes, launcher desktop IDs and environment variables. The native executable
is `build/elsewhere`; the active addon is `world/addons/elsewhere_runtime/`.
Environment overrides now use the `ELSEWHERE_` prefix. Godot UID sidecars move
with their resources, preserving their values. Rendering and input behavior are
otherwise unchanged.

The local Git origin uses `git@github.com:christafford/elsewhere.git`, preserving
SSH authentication. No remote write or host/container rename was performed.
The unrelated untracked `xxx.txt` and the two owner-supplied patch files that
appeared during this work are untouched and excluded from the commit.
Git history, historical capture
pixels, downloaded dependencies and ignored historical logs remain evidence;
historical document text has been normalized to the new project spelling at the
owner's request, without changing its observation dates or revisions.

Existing Meson/CMake caches contained absolute paths to the previous checkout.
The main build, binding build and Godot import cache were preserved under the
ignored `tmp/pre-rename-builds/` and regenerated at the current location. Old
standalone generated binaries were also preserved there. Other ignored build
variants need regeneration before reuse.

## Verification

Agent-run checks on 2026-10-09, base `9c2de0c` plus this rename:

- Fresh `meson setup build --buildtype=debug`, `meson compile -C build -j 2`
  and `meson test -C build --print-errorlogs`: all 34 tests passed.
- `python3 tests/test_desktop_apps.py` and
  `python3 tests/test_godot_launcher.py`: all 8 tests passed.
- `node --test .opencode/tests/auto-continue.test.js`: all 62 tests passed.
- `SDL_JOYSTICK_LINUX_CLASSIC=1 tools/check-godot-binding.sh`: native lifecycle,
  frame, keyboard, pointer, resize and surface-tree checks, fresh Godot class
  discovery and 300 binding calls passed.
- `GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-godot-study.sh`:
  clean import/runtime validation, all 20 Godot trials and 8 Python tests passed.
  This includes movement, renamed room/shader resources, live terminal output,
  keyboard/pointer/resize, launcher and application-panel interactions.
- `GODOT_BINARY="$PWD/.tools/rename/godot-check.sh" tools/check-chrome-application.sh`:
  `CHROME_APPLICATION_OK failures=0`, including real launch, typing, pointer,
  scrolling, resize, second window and browser-initiated window close.
- Shell/Python syntax, relative Markdown links, unique numeric task IDs and
  mechanical substitution review passed. Final source/path scans found no old
  project name in tracked content or paths, and diffs passed whitespace checks.

Logs are ignored `.tools/rename-*.log`. Graphical checks use the private GPU
Weston wrapper `.tools/rename/godot-check.sh` through `GODOT_BINARY`; this does
not replace the host desktop. No new human usability or visual-quality gate is
claimed by a naming change.

## Continuation

This bounded rename does not authorize an autonomous roadmap run. The previous
T45 targeted-close task and browser popup/clipboard limitations remain as
recorded in STATUS.md and handoff 37. Await the owner's next requested change.
