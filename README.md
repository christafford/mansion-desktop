# mansion-desktop

Mansion Desktop is a 3D spatial desktop environment for Linux: a nested Wayland
compositor whose shell is a first-person mansion. This repository is at the
very beginning (Project 1 of the roadmap: a nested compositor that can show
one real terminal).

## Start here

| If you want to… | Read |
| --- | --- |
| understand the idea | [mansion-desktop-concept.md](mansion-desktop-concept.md) |
| see the plan and its acceptance criteria | [PROJECT-ROADMAP.md](PROJECT-ROADMAP.md) |
| pick up the next piece of work | [docs/TASKS.md](docs/TASKS.md), then [docs/STATUS.md](docs/STATUS.md) |
| know what actually works today | [docs/STATUS.md](docs/STATUS.md) |
| build and run | [GETTING_STARTED.md](GETTING_STARTED.md), [DEVELOPMENT.md](DEVELOPMENT.md) |
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
terminal such as `weston-terminal`; note that no client can map a window until
task P1-T03 lands):

```sh
./build/mansion-desktop --launch weston-terminal
```

## Autonomous development

Agents (OpenCode with the bundled auto-continue plugin, or any other) follow
[AGENTS.md](AGENTS.md) and work through `docs/TASKS.md` one task at a time,
proving each task with `meson test` before ticking and committing it.
