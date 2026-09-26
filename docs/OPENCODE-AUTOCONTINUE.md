# OpenCode autonomous runs

The repository ships a dependency-free OpenCode **V2** plugin at
`.opencode/plugins/auto-continue.js`. OpenCode 2.x loads it from
`.opencode/plugins/` when a session is opened in this directory and reloads it
when the file changes. It was written for and verified against OpenCode
2.0.16; the V1 plugin format (`export const Plugin = async ({client}) => hooks`)
no longer loads in 2.x.

## Before an unattended run

1. Make sure the llama.cpp server is up: `curl -s http://127.0.0.1:8080/v1/models`.
2. Build and test once yourself so the run starts green:
   `meson compile -C build && meson test -C build`.
3. Optionally install `weston` (`sudo pacman -S weston`) so the human smoke
   test can be performed later. Autonomous sessions never use `sudo`.
4. Check `git status --short` is clean or contains only work you want the run
   to build on. The run commits after every verified task.
5. Start OpenCode in this directory, then:

   ```text
   /autocontinue Projects 1–4 of docs/TASKS.md
   ```

   The scope is free text; keep it to a task range that exists in
   `docs/TASKS.md`, for example `Project 1 (P1-T03 to P1-T11)`.

## What the plugin does

- `/autocontinue <scope>` sends the run instructions as a prompt and enables
  follow-ups for that session only. Child sessions and other sessions are
  never touched.
- After each turn ends normally, it waits `delayMs` (2 s), reads the transcript,
  and sends `Automatic follow-up N/MAX.` plus the same instructions again with
  `delivery: queue`.
- The instructions tell the model to take the next unchecked task in
  `docs/TASKS.md`, build, run `meson test`, tick the task only when the
  acceptance check passed, update `docs/STATUS.md`, commit, and end with
  `Next: <task id>`, or with an exact final line `AUTOCONTINUE_DONE` (scope
  finished) or `AUTOCONTINUE_BLOCKED` (nothing in scope can proceed).

## Stop conditions

| Condition | Default |
| --- | --- |
| Follow-ups per run (`maxContinuations`) | 400 |
| Wall-clock limit (`maxDurationMs`) | 96 h |
| Final line is `AUTOCONTINUE_DONE` or `AUTOCONTINUE_BLOCKED` | always |
| Identical final replies in a row (`repeatLimit`) | 3 |
| Empty final reply | always |
| Follow-ups without any change to `git status`/`HEAD` (`stallLimit`) | 12 (0 disables) |
| Turn failed or was interrupted; assistant message carries an error | always |
| Permission request (`permission.asked`) or question form (`form.created`) | always |
| Any user prompt that is not the plugin's own follow-up | always |
| `/autostop` | always |
| OpenCode restart or plugin reload | run state is in memory only |

Override defaults with environment variables before starting OpenCode:
`AUTOCONTINUE_MAX_CONTINUATIONS`, `AUTOCONTINUE_MAX_HOURS`,
`AUTOCONTINUE_DELAY_MS`, `AUTOCONTINUE_POLL_MS`, `AUTOCONTINUE_REPEAT_LIMIT`,
`AUTOCONTINUE_STALL_LIMIT`. Plugin options from `opencode.jsonc`
(`"plugins": [{"package": "./.opencode/plugins/auto-continue.js", "options": {...}}]`)
take precedence over both, but the auto-discovered file is already loaded, so
prefer environment variables.

## Take over or stop

- Type any message: follow-ups stop and your message is handled normally.
- `/autostop`: stops follow-ups and asks the model for a short progress report.
- OpenCode's interrupt (Esc) ends the turn as *interrupted*; the plugin stops.
- The permission settings in `opencode.jsonc` still apply. The plugin never
  answers permission prompts or questions; a prompt ends the run.

## Logs

- `.opencode/auto-continue.log` (git-ignored): enable/follow-up/stop lines
  with reasons and counts.
- OpenCode's own log: `opencode debug paths` shows the `log` directory; look
  for `loading plugin` and `failed to load plugin`.

## Internals (for maintainers)

- V2 Promise API: `export default { id, async setup(ctx) { ... return cleanup } }`.
  Uses `ctx.command.transform` (+ `reload`) for the two commands,
  `ctx.session.prompt`, `ctx.session.get`, `ctx.session.context` (transcript
  since the last compaction), `ctx.session.hook("prompt")` for takeover
  detection, and `ctx.event.subscribe()` for `session.execution.*`,
  `permission.asked`, `form.created`, `session.deleted`.
- Turn boundary = newest non-idle message is a completed assistant message
  (`time.completed` set, `finish === "stop"`, or an `idle` message with
  `outcome: succeeded` after it). A poll of `Session.time.idle` every
  `pollMs` (10 s) backs up the event stream.
- Commands registered by plugins do not appear in `GET /api/command`, but
  `POST /api/session/{id}/command` and the TUI `/autocontinue` execute them.

## Verification

Unit tests (Node 20+):

```sh
node --test .opencode/tests/*.test.js
```

Live check performed 2026-09-26 with OpenCode 2.0.16 and the local Qwen3.6
model: a restricted probe session received `/autocontinue` via the API, the
model answered once, the plugin sent follow-up 1/400 after 8 s, the model
replied `AUTOCONTINUE_DONE`, and the plugin logged
`stopped (AUTOCONTINUE_DONE); 1 automatic follow-ups`.

To repeat it in the TUI, open a new session and run:

```text
/autocontinue VERIFICATION ONLY. Do not read, edit, or run anything, and ignore the numbered steps below. First turn: reply with one sentence and no marker. On the automatic follow-up reply with only the line AUTOCONTINUE_DONE.
```

Expect exactly one automatic follow-up, then an idle session.
