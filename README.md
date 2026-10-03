# mansion-desktop

Mansion Desktop is a Linux spatial desktop: real Wayland applications inside a
persistent first-person environment. Current work is Project 21: a Godot world
frontend, the retained C++ compositor core, and a furnished Poly Haven study.
The runnable `build/mansion-desktop --room-camera` command still uses the legacy
custom polygon renderer. The separate Godot study now renders textured furniture
and supports collision-based movement. A real Weston terminal supplies live
output to its monitor. Enter opens a readable keyboard-controlled terminal view;
Ctrl+Alt+Escape returns to the room. Pointer and resize work comes next.
See [current status](docs/STATUS.md) and [Godot launch instructions](world/README.md).

```sh
python3 world/tools/prepare_study_assets.py
tools/validate-godot-project.sh
tools/run-godot.sh --audio-driver Dummy
```

Read [Decision 06](docs/decisions/06-godot-poly-haven.md),
[the art brief](docs/ART-DIRECTION.md) and [the executable tasks](docs/TASKS.md).
For Qwen/OpenCode, use [the launch instructions](docs/OPENCODE-AUTOCONTINUE.md).
The immediate target is completing pointer input, resizing and close/relaunch
in the furnished study, followed by a useful two-room workspace.

## Start here

| If you want to… | Read |
| --- | --- |
| understand the idea | [mansion-desktop-concept.md](mansion-desktop-concept.md) |
| see the plan and its acceptance criteria | [PROJECT-ROADMAP.md](PROJECT-ROADMAP.md) |
| pick up the next piece of work | [docs/TASKS.md](docs/TASKS.md), then [docs/STATUS.md](docs/STATUS.md) |
| know what actually works today | [docs/STATUS.md](docs/STATUS.md) |
| build and run | [GETTING_STARTED.md](GETTING_STARTED.md), [DEVELOPMENT.md](DEVELOPMENT.md) |
| understand target architecture and ownership | [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) |
| perform real-client and visual acceptance | [docs/ACCEPTANCE.md](docs/ACCEPTANCE.md) |
| understand recorded decisions | [docs/decisions/](docs/decisions/) |
| read per-project handoffs | [docs/handoffs/](docs/handoffs/) |
| run OpenCode unattended on this repo | [docs/OPENCODE-AUTOCONTINUE.md](docs/OPENCODE-AUTOCONTINUE.md) |
| write or run tests | [tests/README.md](tests/README.md) |

## Build and test

```sh
./check-deps.sh
meson setup build --buildtype=debug
meson compile -C build
meson test -C build --print-errorlogs
```

Run headless (no display needed):

```sh
./build/mansion-desktop --headless --exit-after-ms 2000
```

Run in a host window and launch a client (needs `DISPLAY` and a Wayland
terminal such as `weston-terminal`):

```sh
./build/mansion-desktop --launch weston-terminal
```

## Autonomous development

Agents (OpenCode with the bundled auto-continue plugin, or any other) follow
[AGENTS.md](AGENTS.md) and work through `docs/TASKS.md` one task at a time,
proving the task's actual acceptance before ticking and committing it. Use the current Next task in `docs/STATUS.md`; distinguish automated checks, real-client evidence and human product
gates. Follow the recovery ranges in docs/OPENCODE-AUTOCONTINUE.md.
