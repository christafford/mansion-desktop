// OpenCode V2 plugin (Promise API). No dependencies; plain JavaScript.
//
// Adds two commands to the session:
//   /autocontinue <scope>   start a task-scoped autonomous run in this session
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

import { appendFile, readFile, writeFile, rename, unlink } from "node:fs/promises";
import { execFile } from "node:child_process";
import path from "node:path";

export const DEFAULTS = {
  maxContinuations: 0,              // 0 = no automatic follow-up count limit
  maxDurationMs: 0,                 // 0 = no wall-clock limit
  delayMs: 2000,                    // settle time after a turn ends
  pollMs: 10000,                    // idle detection fallback poll interval
  repeatLimit: 3,                   // identical final replies before stopping
  stallLimit: 12,                   // follow-ups with no repository change; 0 disables
  falseDoneLimit: 2,                // consecutive unverified DONE claims before stopping
};

const ENV = {
  maxContinuations: ["AUTOCONTINUE_MAX_CONTINUATIONS", 1],
  maxDurationMs: ["AUTOCONTINUE_MAX_HOURS", 60 * 60 * 1000],
  delayMs: ["AUTOCONTINUE_DELAY_MS", 1],
  pollMs: ["AUTOCONTINUE_POLL_MS", 1],
  repeatLimit: ["AUTOCONTINUE_REPEAT_LIMIT", 1],
  stallLimit: ["AUTOCONTINUE_STALL_LIMIT", 1],
  falseDoneLimit: ["AUTOCONTINUE_FALSE_DONE_LIMIT", 1],
};

export const DONE = "AUTOCONTINUE_DONE";
export const BLOCKED = "AUTOCONTINUE_BLOCKED";

export function instructions(scope) {
  return `Autonomous run scope: ${scope}
This is an unattended run. Decide routine implementation details and continue.
No arbitrary deadline applies; task acceptance, quality and verified evidence
determine completion. Follow the configured stop conditions and user controls.
1. Read AGENTS.md, docs/STATUS.md, docs/TASKS.md, docs/ARCHITECTURE.md and the
   current decision (06-godot-poly-haven.md). For Project 21 also read
   docs/GODOT-INTEGRATION.md, docs/ART-DIRECTION.md and docs/ASSET-PIPELINE.md.
   Select the first eligible unchecked non-human task in scope. Prerequisites
   require both checked boxes and their stated evidence. Record blocked tasks
   and continue another independent eligible task. Stay inside this scope.
2. Implement one coherent task at a time. It can span several turns; checkpoint
   the exact next step and continue. Split by meaningful acceptance boundaries
   into unique unsuffixed numeric IDs if needed. Do not replace implementation
   with more planning, TODOs, synthetic substitutes or weakened acceptance.
3. Run task-specific checks and relevant regressions: Meson for C++, engine
   import/script/runtime checks for Godot, extension/real-client checks for the
   bridge, manifest/import checks for assets, actual rendered image inspection
   for art, Node tests for this plugin, links/references/diff for documentation.
   Fix regressions before advancing. A headless import is not visual evidence.
4. Only after the full acceptance passes, tick the task, record revision,
   environment, commands/results and evidence in STATUS/ACCEPTANCE/handoff,
   then stage only this task's files and commit locally (never push).
   Never use git add -A to sweep unrelated user work into the commit.
5. If unfinished, leave a buildable checkpoint and exact continuation point in
   STATUS. Do not tick missing features or human observation. Label agent-observed
   GUI evidence honestly. Keep shm-only/GPU and target-device limits explicit.
Never claim a check passed without running it. Preserve test strength, user work
and host services. Re-read the on-disk state after compaction/restart. Resolve
recoverable failures; do not loop indefinitely on a proven unavailable tool.
End with exactly ${DONE} only when every non-human task in scope has passed,
or ${BLOCKED} when no eligible task in scope can proceed (record blockers first),
otherwise "Next: <task id>". Markers go on the final line outside code blocks.
The plugin verifies checklist completion only, not evidence or prerequisites;
you must verify both before ticking or claiming completion.`;
}

// Task ids look like P1-T03; "(human)" tasks are never required for DONE.
const TASK_RE = /^- \[( |x|X)\] \*\*(P(\d+)-T(\d+))\b([^\n]*)/gm;

// Which projects and tasks a free-text scope covers. Understands "Project 3",
// "Projects 1–4" / "1-4" / "1 to 4", and explicit ids "P1-T03" or "P1-T03 to
// P1-T11". Returns null when nothing recognisable is present. Such scopes are
// rejected instead of accepting an unverified DONE claim.
export function parseScope(scope) {
  const text = String(scope ?? "");
  const projects = new Set();
  const taskRanges = [];
  const range = (a, b) => { for (let n = Math.min(a, b); n <= Math.max(a, b); n++) projects.add(n); };
  for (const m of text.matchAll(/\bprojects?\s+(\d+)(?:\s*(?:[-–—]|to|through)\s*(\d+))?/gi)) {
    range(Number(m[1]), m[2] ? Number(m[2]) : Number(m[1]));
  }
  for (const m of text.matchAll(/\bP(\d+)-T(\d+)\b(?:\s*(?:[-–—]|to|through)\s*P(\d+)-T(\d+)\b)?/gi)) {
    const from = [Number(m[1]), Number(m[2])];
    const to = m[3] ? [Number(m[3]), Number(m[4])] : from;
    taskRanges.push({ from, to });
  }
  if (!projects.size && !taskRanges.length) return null;
  return { projects, taskRanges };
}

const cmp = (a, b) => a[0] - b[0] || a[1] - b[1];

// Match the same task grammar for validation and completion. Historical
// suffixed IDs remain aliases; new executable tasks use unsuffixed numeric IDs.
function tasksInScope(tasksMarkdown, parsed) {
  const matches = [];
  for (const m of tasksMarkdown.matchAll(TASK_RE)) {
    const [, tick, id, project, task, rest] = m;
    const key = [Number(project), Number(task)];
    const inScope = parsed.projects.has(key[0]) ||
      parsed.taskRanges.some((r) => cmp(r.from, key) <= 0 && cmp(key, r.to) <= 0);
    if (inScope) matches.push({ tick, id, rest });
  }
  return matches;
}

// Tasks inside the scope that are not ticked and not marked (human).
export function unfinishedTasks(tasksMarkdown, scope) {
  const parsed = typeof scope === "string" ? parseScope(scope) : scope;
  if (!parsed) return null;
  return tasksInScope(tasksMarkdown, parsed)
    .filter(({ tick, rest }) => tick.toLowerCase() !== "x" && !/\(human\)/i.test(rest))
    .map(({ id }) => id);
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
    const min = ["maxContinuations", "maxDurationMs", "stallLimit", "pollMs"].includes(key) ? 0 : 1;
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

// Run state lives on disk so a plugin reload or OpenCode restart resumes the
// run instead of silently ending it. Timers and in-flight flags are not saved.
const PERSISTED = ["scope", "started", "count", "repeats", "stalls", "expected", "lastIdle",
  "fingerprint", "lastMessage", "lastText", "falseDone"];

function fileStateStore(file) {
  return {
    async load() {
      try {
        return JSON.parse(await readFile(file, "utf8"));
      } catch (error) {
        if (error?.code === "ENOENT") return {};
        throw error;
      }
    },
    async save(state) {
      if (!Object.keys(state).length) {
        await unlink(file).catch(() => {});
        return;
      }
      const tmp = `${file}.tmp`;
      await writeFile(tmp, JSON.stringify(state, null, 2));
      await rename(tmp, file);
    },
  };
}

// `deps` lets tests replace the repository fingerprint, the clock, the task
// list reader, and the state store.
export function createPlugin(deps = {}) {
  const fingerprint = deps.fingerprint ?? repoFingerprint;
  const now = deps.now ?? Date.now;
  const readTasks = deps.readTasks ?? ((directory) => readFile(path.join(directory, "docs", "TASKS.md"), "utf8"));
  const stateStore = deps.stateStore ?? fileStateStore;

  return {
    id: "auto-continue",

    async setup(ctx) {
      const limits = resolveLimits(ctx.options ?? {});
      const directory = ctx.location?.directory ?? process.cwd();
      const logFile = path.join(directory, ".opencode", "auto-continue.log");
      const store = stateStore(path.join(directory, ".opencode", "auto-continue.state.json"));
      const runs = new Map();
      const controller = new AbortController();
      let pollTimer;
      let unloading = false;

      async function log(message) {
        const line = `${new Date().toISOString()} ${message}`;
        console.log(`[auto-continue] ${message}`);
        try {
          await appendFile(logFile, line + "\n");
        } catch {
          /* the log file is a convenience; never fail the run over it */
        }
      }

      let persisting = Promise.resolve();
      function persist() {
        const state = {};
        for (const [id, run] of runs) {
          state[id] = Object.fromEntries(PERSISTED.filter((k) => run[k] !== undefined).map((k) => [k, run[k]]));
        }
        persisting = persisting
          .then(() => store.save(state))
          .catch((error) => log(`state not saved: ${error?.message ?? error}`));
        return persisting;
      }

      function stop(id, reason) {
        const run = runs.get(id);
        if (!run) return;
        clearTimeout(run.timer);
        runs.delete(id);
        void log(`${id}: stopped (${reason}); ${run.count} automatic follow-ups`);
        // An unload keeps the state file so the run resumes on reload.
        if (!unloading) void persist();
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

      async function validatedTasks(scope) {
        const parsed = parseScope(scope);
        if (!parsed) throw new Error("Scope must name a numeric project or task range in docs/TASKS.md");
        const markdown = await readTasks(directory);
        if (!tasksInScope(markdown, parsed).length) {
          throw new Error(`Scope "${scope}" matches no tasks in docs/TASKS.md`);
        }
        return unfinishedTasks(markdown, parsed);
      }

      // Re-read on DONE: missing/deleted tasks cannot turn a claim into success.
      async function verifyDone(scope) {
        return validatedTasks(scope);
      }

      const countLabel = (count) => `${count}/${limits.maxContinuations || "unlimited"}`;

      async function check(id, run) {
        if (runs.get(id) !== run || run.checking) return;
        run.checking = true;
        try {
          if ((limits.maxDurationMs > 0 && now() - run.started >= limits.maxDurationMs) ||
              (limits.maxContinuations > 0 && run.count >= limits.maxContinuations)) {
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
          if (marker === BLOCKED) {
            stop(id, marker);
            return;
          }
          let falseDone = "";
          if (marker === DONE) {
            const open = await verifyDone(run.scope);
            if (runs.get(id) !== run) return;
            if (!open.length) {
              stop(id, marker);
              return;
            }
            run.falseDone = (run.falseDone ?? 0) + 1;
            void log(`${id}: ${DONE} claimed but ${open.length} task(s) in scope are unticked: ${open.join(", ")} (claim ${run.falseDone}/${limits.falseDoneLimit})`);
            if (run.falseDone >= limits.falseDoneLimit) {
              stop(id, `${DONE} claimed ${run.falseDone} times with unticked tasks: ${open.join(", ")}`);
              return;
            }
            falseDone = `Your previous reply ended with ${DONE}, but docs/TASKS.md still has unticked tasks in scope: ${open.join(", ")}. The run is not finished. Do not repeat that marker until every one of them is ticked with its acceptance check passed.\n`;
          } else {
            run.falseDone = 0;
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
          run.expected = `Automatic follow-up ${countLabel(run.count)}.\n${falseDone}${instructions(run.scope)}`;
          await persist();
          await ctx.session.prompt({ sessionID: id, text: run.expected, delivery: "queue" });
          void log(`${id}: follow-up ${countLabel(run.count)} sent`);
        } catch (error) {
          if (runs.get(id) === run) stop(id, `continuation failed: ${error?.message ?? error}`);
        } finally {
          run.checking = false;
        }
      }

      async function start(sessionID, scope, delivery) {
        const session = await ctx.session.get({ sessionID });
        if (!session || session.parentID) throw new Error("Auto-continue requires a top-level session");
        await validatedTasks(scope);
        stop(sessionID, "restarting run");
        const text = instructions(scope);
        runs.set(sessionID, {
          scope, started: now(), count: 0, repeats: 0, stalls: 0, falseDone: 0,
          expected: text, checking: false, lastIdle: session.time?.idle,
          fingerprint: limits.stallLimit > 0 ? await fingerprint(directory) : undefined,
        });
        await persist();
        await log(`${sessionID}: enabled, follow-up limit ${limits.maxContinuations || "unlimited"}, wall-clock limit ${limits.maxDurationMs || "unlimited"}`);
        await ctx.session.prompt({ sessionID, text, delivery: delivery ?? "queue" });
      }

      await ctx.command.transform((editor) => {
        editor.add({
          name: "autocontinue",
          description: "Start a task-scoped autonomous run: /autocontinue <scope>",
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

      // Resume runs saved by a previous load of this plugin.
      try {
        const saved = await store.load();
        for (const [id, data] of Object.entries(saved)) {
          let session;
          try {
            session = await ctx.session.get({ sessionID: id });
          } catch {
            session = undefined;
          }
          if (!session) {
            void log(`${id}: saved run dropped, session no longer exists`);
            continue;
          }
          try {
            await validatedTasks(data.scope);
          } catch (error) {
            void log(`${id}: saved run dropped, invalid scope/task list: ${error?.message ?? error}`);
            continue;
          }
          const run = { ...data, checking: false, timer: undefined };
          runs.set(id, run);
          void log(`${id}: resumed after reload; ${run.count} follow-ups so far`);
          // The turn may have ended while we were unloaded: check right away.
          schedule(id, run);
        }
        await persist();
      } catch (error) {
        void log(`saved state unreadable, starting clean: ${error?.message ?? error}`);
      }

      return () => {
        unloading = true;
        controller.abort();
        clearInterval(pollTimer);
        for (const id of runs.keys()) stop(id, "plugin unloaded; state kept for resume");
      };
    },
  };
}

export default createPlugin();
