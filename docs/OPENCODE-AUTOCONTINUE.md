# OpenCode autonomous runs

The repository includes a dependency-free project plugin at
`.opencode/plugins/auto-continue.js`. OpenCode loads project plugins at startup.
Restart OpenCode in this repository after pulling or changing the plugin.

## Start and stop

In OpenCode, enter:

```text
/autocontinue Complete Projects 1–4 of PROJECT-ROADMAP.md
```

Supply an explicit scope. The command uses the build agent and authorizes work
only within that scope. Each time the session finishes a normal response, the
plugin waits 1.5 seconds and sends another prompt to the same session, reminding
it of the scope, verification requirements, status file, and handoffs.

To disable follow-ups:

```text
/autostop
```

Any other new user message also disables follow-ups, so you can take over normally.
Use OpenCode's interrupt control to interrupt ongoing work, then `/autostop` if
needed. The plugin stops on reported session/assistant errors, including aborts.
`/autostop` itself disables future prompts; it is not a process-kill command.
Run `/autocontinue <scope>` again to resume deliberately.

## Boundaries

- At most 20 automatic follow-ups, in addition to the initial command.
- No further follow-ups after eight hours. This does not interrupt an in-flight
  task, impose a token/spending cap, or guarantee an eight-hour process lifetime.
- Runs are held in memory and are disabled after OpenCode restarts.
- Only explicitly started top-level sessions are continued. Child sessions and
  unrelated sessions are ignored.
- Permission/question requests and API errors disable continuation. The plugin
  never approves permissions or retries failed requests automatically.
- Three identical final responses, or an empty final response, stop the run.
  This is a simple repetition check, not proof of implementation progress.
- Completion and blocker detection rely on the assistant's final response. The
  plugin instructs it to finish with an exact final line `AUTOCONTINUE_DONE` when
  the entire scope is verified, or `AUTOCONTINUE_BLOCKED` when no eligible work
  can proceed. It does not independently certify the work or acceptance tests.
- Compaction summaries, unfinished replies, and tool-only turns do not trigger
  additional prompts. Normal OpenCode compaction can proceed independently.

Limits are defined in `DEFAULTS` at the top of the plugin. Change them and restart
OpenCode if needed. Stop reasons and continuation counts are written to OpenCode's
application log under the `auto-continue` service.

The root `opencode.jsonc` uses OpenCode's `permission` object with `bash` and `task`
tool names. Existing repository restrictions still apply. Permission settings
control tool access; the plugin supplies subsequent turns.

## Verification

Verified with OpenCode 1.18.32: configuration/plugin loading, command registration,
and a live server run against a local mock model produced exactly one automatic
follow-up and then stopped on `AUTOCONTINUE_DONE`. No paid model calls were used.
The unit suite covers cancellation, session isolation, duplicate events, limits,
completion/blocker markers, and SDK failures.

Run the dependency-free tests from the repository root with Node.js 20 or newer:

```sh
node --test .opencode/tests/*.test.js
```

For a live check, start a new OpenCode session and run:

```text
/autocontinue Read README.md without editing files. In the first turn report its purpose and the next action, without a completion marker. On the automatic follow-up report completion and finish with AUTOCONTINUE_DONE.
```

Expect one automatic follow-up and then an idle session. Test `/autostop` and a
normal user message to confirm takeover in your installed OpenCode version.

API references: [plugins](https://opencode.ai/docs/plugins/),
[SDK](https://opencode.ai/docs/sdk/),
[permissions](https://opencode.ai/docs/permissions/).
