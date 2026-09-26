// OpenCode V2 plugin (Promise API). No dependencies; plain JavaScript.
//
// Adds two commands to the session:
//   /autocontinue <scope>   start a bounded autonomous run in this session
//   /autostop               disable further automatic follow-ups
//
// After every completed assistant turn in an enabled session, the plugin waits
// briefly and sends another user prompt that restates the scope and the working
// rules. It stops on run limits, completion/blocker markers, repeated or empty
// replies, turns that fail or are interrupted, permission/question prompts,
// new user input, and a long streak of turns that leave the repository untouched.
//
// OpenCode 2.x loads this file from .opencode/plugins/ and reloads it on change.
// Settings come from DEFAULTS, then environment variables, then ctx.options.

import { appendFile } from "node:fs/promises";
import { execFile } from "node:child_process";
import path from "node:path";

export const DEFAULTS = {
  maxContinuations: 400,            // automatic follow-ups per run
  maxDurationMs: 96 * 60 * 60 * 1000, // no follow-ups after this much wall time
  delayMs: 2000,                    // settle time after a turn ends
  pollMs: 10000,                    // idle detection fallback poll interval
  repeatLimit: 3,                   // identical final replies before stopping
  stallLimit: 12,                   // follow-ups with no repository change; 0 disables
};

const ENV = {
  maxContinuations: ["AUTOCONTINUE_MAX_CONTINUATIONS", 1],
  maxDurationMs: ["AUTOCONTINUE_MAX_HOURS", 60 * 60 * 1000],
  delayMs: ["AUTOCONTINUE_DELAY_MS", 1],
  pollMs: ["AUTOCONTINUE_POLL_MS", 1],
  repeatLimit: ["AUTOCONTINUE_REPEAT_LIMIT", 1],
  stallLimit: ["AUTOCONTINUE_STALL_LIMIT", 1],
};

export const DONE = "AUTOCONTINUE_DONE";
export const BLOCKED = "AUTOCONTINUE_BLOCKED";

export function instructions(scope) {
  return `Autonomous run scope: ${scope}
This is an unattended run. Nobody will answer questions, so decide and continue.
1. Read docs/STATUS.md ("Next task") and docs/TASKS.md. Take the first unchecked
   task inside the scope whose prerequisites are checked.
2. Do only that task, with small changes that fit the existing code.
3. Run: meson compile -C build && meson test -C build --print-errorlogs
   Fix failures before anything else.
4. Only when the task's acceptance check has actually passed: tick it in
   docs/TASKS.md, update docs/STATUS.md, then commit:
   git add -A && git commit -q -m "<task id>: <summary>"   (never push)
5. If the task cannot be finished this turn, leave the tree compiling and write
   the exact continuation point under "Next task" in docs/STATUS.md.
Never claim a check passed without running it. Never tick a task whose check
failed. Do not delete or weaken tests. Stay inside the scope.
End your reply with a line containing exactly ${DONE} when every task in the
scope is ticked and verified, ${BLOCKED} when no task in the scope can proceed
(record the blockers in docs/STATUS.md first), otherwise "Next: <task id>".
Put these markers outside code blocks.`;
}

export function resolveLimits(options = {}, env = process.env) {
  const limits = { ...DEFAULTS };
  for (const [key, [name, scale]] of Object.entries(ENV)) {
    if (env[name] !== undefined && env[name] !== "") {
      const value = Number(env[name]);
      if (!Number.isFinite(value)) throw new Error(`auto-continue: ${name} must be a number`);
      limits[key] = Math.round(value * scale);
    }
  }
  for (const key of Object.keys(DEFAULTS)) {
    if (options[key] !== undefined) limits[key] = Number(options[key]);
    const min = key === "stallLimit" || key === "pollMs" ? 0 : 1;
    if (!Number.isSafeInteger(limits[key]) || limits[key] < min) {
      throw new Error(`auto-continue: ${key} must be an integer >= ${min}`);
    }
  }
  return limits;
}

// Text of an assistant message: V2 `content` parts, or V1-style `parts`.
export function assistantText(message) {
  const parts = message?.content ?? message?.parts ?? [];
  return parts
    .filter((p) => p && p.type === "text" && typeof p.text === "string" && !p.ignored)
    .map((p) => p.text)
    .join("\n")
    .trim();
}

function lastLine(text) {
  const lines = text.split(/\r?\n/).map((l) => l.trim()).filter(Boolean);
  return lines.at(-1) ?? "";
}

function sessionOf(event) {
  const d = event?.data ?? event?.properties ?? {};
  return d.sessionID ?? d.info?.sessionID ?? event?.sessionID ?? undefined;
}

function repoFingerprint(directory) {
  const git = (args) =>
    new Promise((resolve) => {
      execFile("git", ["-C", directory, ...args], { timeout: 15000 }, (error, stdout) => {
        resolve(error ? `error:${error.code ?? error.message}` : stdout);
      });
    });
  return Promise.all([git(["rev-parse", "HEAD"]), git(["status", "--porcelain"])]).then((parts) =>
    parts.join("\n"),
  );
}

// `deps` lets tests replace the repository fingerprint and the clock.
export function createPlugin(deps = {}) {
  const fingerprint = deps.fingerprint ?? repoFingerprint;
  const now = deps.now ?? Date.now;

  return {
    id: "auto-continue",

    async setup(ctx) {
      const limits = resolveLimits(ctx.options ?? {});
      const directory = ctx.location?.directory ?? process.cwd();
      const logFile = path.join(directory, ".opencode", "auto-continue.log");
      const runs = new Map();
      const controller = new AbortController();
      let pollTimer;

      async function log(message) {
        const line = `${new Date().toISOString()} ${message}`;
        console.log(`[auto-continue] ${message}`);
        try {
          await appendFile(logFile, line + "\n");
        } catch {
          /* the log file is a convenience; never fail the run over it */
        }
      }

      function stop(id, reason) {
        const run = runs.get(id);
        if (!run) return;
        clearTimeout(run.timer);
        runs.delete(id);
        void log(`${id}: stopped (${reason}); ${run.count} automatic follow-ups`);
      }

      function schedule(id, run) {
        clearTimeout(run.timer);
        run.timer = setTimeout(() => void check(id, run), limits.delayMs);
        run.timer.unref?.();
      }

      async function messagesOf(id) {
        const result = await ctx.session.context({ sessionID: id });
        const list = Array.isArray(result) ? result : result?.data;
        if (!Array.isArray(list)) throw new Error("session.context returned no message list");
        return [...list].sort((a, b) => (a.time?.created ?? 0) - (b.time?.created ?? 0));
      }

      async function check(id, run) {
        if (runs.get(id) !== run || run.checking) return;
        run.checking = true;
        try {
          if (now() - run.started >= limits.maxDurationMs || run.count >= limits.maxContinuations) {
            stop(id, "run limit reached");
            return;
          }
          const messages = await messagesOf(id);
          if (runs.get(id) !== run) return;

          const last = messages.at(-1);
          if (!last) return;
          if (last.type === "idle" && last.outcome !== "succeeded") {
            stop(id, `turn ${last.outcome}`);
            return;
          }
          // The turn is over only when the newest non-idle message is a completed
          // assistant reply. Anything else means work is still in flight.
          const latest = [...messages].reverse().find((m) => m.type !== "idle");
          if (!latest || latest.type !== "assistant") return;
          if (latest.error) {
            stop(id, `assistant error: ${latest.error.message ?? latest.error.type ?? "unknown"}`);
            return;
          }
          if (!latest.time?.completed) return;
          // Without an idle marker after the reply, only a natural stop counts as final.
          if (last.type !== "idle" && latest.finish && latest.finish !== "stop") return;
          if (latest.id === run.lastMessage) return;

          const lastUser = [...messages].reverse().find((m) => m.type === "user");
          if (lastUser && run.expected !== undefined && lastUser.text.trim() !== run.expected.trim()) {
            stop(id, "new user input");
            return;
          }

          const text = assistantText(latest);
          const marker = lastLine(text);
          if (marker === DONE || marker === BLOCKED) {
            stop(id, marker);
            return;
          }
          run.repeats = text === run.lastText ? run.repeats + 1 : 1;
          run.lastText = text;
          if (!text || run.repeats >= limits.repeatLimit) {
            stop(id, "empty or repeated response");
            return;
          }
          if (limits.stallLimit > 0) {
            const print = await fingerprint(directory);
            if (runs.get(id) !== run) return;
            run.stalls = print === run.fingerprint ? run.stalls + 1 : 0;
            run.fingerprint = print;
            if (run.stalls >= limits.stallLimit) {
              stop(id, `no repository change in ${run.stalls} follow-ups`);
              return;
            }
          }

          run.lastMessage = latest.id;
          run.count++;
          run.expected = `Automatic follow-up ${run.count}/${limits.maxContinuations}.\n${instructions(run.scope)}`;
          await ctx.session.prompt({ sessionID: id, text: run.expected, delivery: "queue" });
          void log(`${id}: follow-up ${run.count}/${limits.maxContinuations} sent`);
        } catch (error) {
          if (runs.get(id) === run) stop(id, `continuation failed: ${error?.message ?? error}`);
        } finally {
          run.checking = false;
        }
      }

      async function start(sessionID, scope, delivery) {
        stop(sessionID, "restarting run");
        const session = await ctx.session.get({ sessionID });
        if (!session || session.parentID) throw new Error("Auto-continue requires a top-level session");
        const text = instructions(scope);
        runs.set(sessionID, {
          scope, started: now(), count: 0, repeats: 0, stalls: 0,
          expected: text, checking: false, lastIdle: session.time?.idle,
          fingerprint: limits.stallLimit > 0 ? await fingerprint(directory) : undefined,
        });
        await log(`${sessionID}: enabled, at most ${limits.maxContinuations} automatic follow-ups`);
        await ctx.session.prompt({ sessionID, text, delivery: delivery ?? "queue" });
      }

      await ctx.command.transform((editor) => {
        editor.add({
          name: "autocontinue",
          description: "Start a bounded autonomous run: /autocontinue <scope>",
          execute: async ({ sessionID, prompt, delivery }) => {
            const scope = (prompt?.text ?? "").trim();
            if (!scope) throw new Error("Usage: /autocontinue <scope>, for example: Projects 1–2 of docs/TASKS.md");
            await start(sessionID, scope, delivery);
          },
        });
        editor.add({
          name: "autostop",
          description: "Disable automatic follow-ups for this session",
          execute: async ({ sessionID, delivery }) => {
            const had = runs.has(sessionID);
            stop(sessionID, "user requested stop");
            await ctx.session.prompt({
              sessionID,
              text: had
                ? "Automatic follow-ups are disabled. Briefly report current progress and stop."
                : "No automatic run was active in this session.",
              delivery: delivery ?? "queue",
            });
          },
        });
      });
      // Transforms registered after startup only apply once the domain is recomputed.
      if (typeof ctx.command.reload === "function") await ctx.command.reload();

      // Any prompt that is not our own follow-up hands control back to the user.
      if (typeof ctx.session.hook === "function") {
        await ctx.session.hook("prompt", (event) => {
          const id = event?.sessionID ?? sessionOf(event);
          const run = id ? runs.get(id) : undefined;
          if (!run) return;
          const text = event?.prompt?.text ?? "";
          if (text.trim() !== run.expected?.trim()) stop(id, "new user input");
          else clearTimeout(run.timer);
        });
      }

      // Event stream: primary signal for turn boundaries and hard stops.
      if (typeof ctx.event?.subscribe === "function") {
        void (async () => {
          try {
            for await (const event of ctx.event.subscribe({ signal: controller.signal })) {
              const id = sessionOf(event);
              const run = id ? runs.get(id) : undefined;
              if (!run) continue;
              switch (event.type) {
                case "session.execution.started":
                  clearTimeout(run.timer);
                  break;
                case "session.execution.succeeded":
                  schedule(id, run);
                  break;
                case "session.execution.failed":
                case "session.execution.interrupted":
                case "session.deleted":
                case "permission.asked":
                case "form.created":
                  stop(id, event.type);
                  break;
                default:
                  break;
              }
            }
          } catch (error) {
            if (!controller.signal.aborted) void log(`event stream ended: ${error?.message ?? error}`);
          }
        })();
      }

      // Poll fallback: detects idleness through Session.time.idle even if events are missed.
      async function poll() {
        for (const [id, run] of runs) {
          try {
            const info = await ctx.session.get({ sessionID: id });
            const idle = info?.time?.idle;
            if (idle && idle !== run.lastIdle) {
              run.lastIdle = idle;
              schedule(id, run);
            }
          } catch (error) {
            void log(`${id}: poll failed: ${error?.message ?? error}`);
          }
        }
      }
      if (limits.pollMs > 0) {
        pollTimer = setInterval(() => void poll(), limits.pollMs);
        pollTimer.unref?.();
      }

      await log(`loaded; limits ${JSON.stringify(limits)}`);

      return () => {
        controller.abort();
        clearInterval(pollTimer);
        for (const id of runs.keys()) stop(id, "plugin unloaded");
      };
    },
  };
}

export default createPlugin();
