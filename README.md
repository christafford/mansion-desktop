# mansion-desktop

Mansion Desktop is a 3D spatial desktop environment for Linux: a nested Wayland
compositor whose shell is a first-person mansion. This repository is at the
foundation-recovery stage: the archive contains a basic room, input modes and
synthetic-client tests, while real-application usability remains unaccepted.
The next target is a real terminal on a monitor in one presentable study.
See Decision 05; no new capability is implied by the documentation update.

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
proving the task's actual acceptance before ticking and committing it. Start at
P4-T06; distinguish automated checks, real-client evidence and human product
gates. Follow the recovery ranges in docs/OPENCODE-AUTOCONTINUE.md.
