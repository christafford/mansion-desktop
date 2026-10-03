# Qwen/OpenCode: build the Godot and Poly Haven workspace

The project uses its own `/autocontinue` plugin. This is automatic continuation
after completed agent turns, not editor autocomplete. The owner's chosen Qwen
model/provider is unchanged. No additional agent framework is required.

## Current recovery overrides the historical run instructions below

As of 2026-10-03, do not start another broad T00–T19 unattended run. The owner
requested direct recovery after that workflow produced incorrect completion
claims. Follow STATUS.md and the bounded T36–T40 evidence and next T41 task. After recovery,
assign one eligible task at a time and review the actual acceptance artifacts.
The plugin does not enforce dependencies or determine whether a render is useful.
The broad-run setup below is retained for reference, not the current work order.

## Start the authorized run

Use `/autostop` on any old run before changing its scope. Start OpenCode from
this repository and keep it and the local model server running. The 2026-10-02
audit reopened false completion claims; a saved older scope does not change
automatically when documentation changes. Check environment/plugin-option overrides.

Before starting OpenCode, set these values in the same shell to make the owner's
no-deadline preference explicit:

```sh
export AUTOCONTINUE_MAX_HOURS=0
export AUTOCONTINUE_MAX_CONTINUATIONS=0
opencode
```

Zero disables those two limits; it is also their new default. Positive values
remain available for optional bounded runs. Plugin options override environment
variables, so remove positive option overrides if present. The plugin's existing
OpenCode V2 API is retained; its compatibility was historically reported with
OpenCode 2.0.16. These patches have unit tests, not a new live OpenCode/model run.
Check the actual installed version and plugin load log if commands are absent.

In OpenCode, start the first furnished live-terminal milestone with this exact
command (keep the scope short so additional project names do not broaden it):

```text
/autocontinue P21-T00 through P21-T19
```

The plugin supplies its task loop and reads STATUS.md/TASKS.md. Both now point
at reopened **P21-T00**, then the scaffold, complete asset packages, furnished
room and working bridge. T00/T01/T02/T03/T10 must be proved again; old checkmarks
and "blocked on a display server" handoffs are superseded. Follow each task's
numbered actions and Acceptance line. Do not make the model infer the next task
from the old chronological status history.

The milestone ends only when **a real terminal works in the furnished Godot
study (T19)**. T09's room alone or a compiled `.so` is not completion. The
legacy `build/mansion-desktop --room-camera` executable is not the Godot frontend.
T20–T35 remain later work; start a broader scope only when you intend that work.

Do not use `Projects 1–21`: it includes deferred legacy work. Do not put
`Project 21` in the same scope string as `P21-T00 through P21-T19`: the parser
unions scope expressions, so that would select the whole project. New task IDs
must be unique, numeric, unsuffixed and at column zero. If work is split into
IDs outside the configured range, expand the scope deliberately; the plugin
will not infer this change from dependencies.

## What the agent should accomplish

Project 21 contains explicit prerequisites and full acceptance checks. World art
and core extraction progress independently. P21-T19 requires the furnished room
and a genuinely live software-rendered terminal together before later workspace
features. It does not require direct GPU import. P21-T33 reviews all autonomous
work before owner review. P21-T34 is human; P21-T35 requires its evidence, so a
full Project 21 run eventually stops blocked for that final observation instead
of silently declaring the product accepted. Once the owner records the trial,
resume Project 21 to resolve issues and record the acceptance/next scope.

Tasks can span turns. A task needs its acceptance, not a time quota. The model
should split genuinely large tasks into new numeric tasks with updated dependencies.
It must not split off difficult acceptance requirements and mark a shell of the
original task complete. A document, screenshot replay or mock is not a live
application; successful import is not a visually excellent room. Fix and inspect
until the stated criteria pass. GUI/tool/hardware blockers remain explicit while
other eligible work continues.

## Continuation behavior and limits

After a completed assistant turn, the plugin waits two seconds, then queues a
follow-up with the scope and task loop. It persists session scope/counters in
`.opencode/auto-continue.state.json` and resumes valid existing sessions after
reload. Startup rejects unrecognized scopes, scopes matching no tasks and
unreadable task lists. Saved invalid scopes are dropped. On a DONE claim it
re-reads the task list and verifies all scoped non-human boxes are ticked.

This is checklist verification only. The plugin does not inspect evidence,
understand dependencies, render the room or judge the art. The agent must do
those checks. Human tasks are excluded from its checkbox completion check;
P21-T35 keeps final owner acceptance as an explicit dependent non-human gate.

| Stop condition | Default |
| --- | --- |
| Follow-up count / elapsed time | Unlimited (`0`), optionally positive |
| `AUTOCONTINUE_DONE` with all non-human scoped tasks ticked | Stop |
| `AUTOCONTINUE_BLOCKED` on the final line | Stop |
| Repeated false DONE claims | Stop after 2 |
| Repeated identical final replies | Stop after 3 |
| No repository fingerprint change | Stop after 12 follow-ups (`0` disables) |
| Empty reply, failed/interrupted turn, assistant error | Stop |
| Permission request or question form | Stop |
| New user message, Esc/interruption, `/autostop` | Stop/handoff |

The repository fingerprint uses Git HEAD/status; it is a heuristic, not a proof
of useful progress. Keep exact continuation state up to date. If a long legitimate
task hits this guard, inspect the log and checkpoint, then restart its scope;
do not disable error or repeat protections merely to conceal a loop. Follow-up
budgets are checked between turns, not hard interrupts of running tools.

Unlimited follow-ups do not defeat model context limits, server failures, sleep,
power loss, missing permissions, disk capacity or network availability. Use
on-disk handoffs and restart a valid session when necessary. Existing permission
rules remain in force; this plugin never approves prompts or changes host services.
Pinned repository-local tools/assets are authorized by Decision 06. Missing
system packages are recorded precisely, with independent tasks pursued first.

Environment overrides: `AUTOCONTINUE_MAX_CONTINUATIONS`, `AUTOCONTINUE_MAX_HOURS`,
`AUTOCONTINUE_DELAY_MS`, `AUTOCONTINUE_POLL_MS`, `AUTOCONTINUE_REPEAT_LIMIT`,
`AUTOCONTINUE_STALL_LIMIT`, `AUTOCONTINUE_FALSE_DONE_LIMIT`. Keep delay/repeat/false
DONE values positive; count/duration/poll/stall permit zero. Configuration
options take precedence. Do not register the auto-discovered file twice.

## Inspect, stop and resume

Use `.opencode/auto-continue.log` for load, follow-up, stop and resume reasons;
use OpenCode's own logs for plugin-load failures. `/autostop` disables follow-ups
and requests a progress report. Any ordinary user message also takes over, so
don't send status questions if you want the uninterrupted run to continue.

If the service/process dies, start it again and inspect STATUS/task evidence plus
the continuation log. Saved sessions may resume automatically. Don't launch a
second worker against the same worktree while an old run is active. A new scope
restarts counters; the unlimited preference remains explicit in the shell.
No multi-agent work or external messaging is required by this plan.

## Verification

From the repository root:

```sh
node --test .opencode/tests/*.test.js
```

If the execution environment restricts test subprocesses and supports the flag:

```sh
node --test --test-isolation=none .opencode/tests/*.test.js
```

The 2026-10-02 documentation audit ran 62 passing plugin tests, including optional
positive limits, unlimited continuation past the former limits after reload,
scope rejection, missing/deleted task lists, takeover, failure/stall controls,
and visibility of the human-dependent final gate. No new product C++/Godot
capability is implied by those automation tests.
