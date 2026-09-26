const DEFAULTS = { maxContinuations: 20, maxDurationMs: 8 * 60 * 60 * 1000, delayMs: 1500 };
const textOf = (parts) => parts.filter((p) => p.type === "text" && !p.ignored).map((p) => p.text).join("\n");

function instructions(scope) {
  return `Autonomous run scope: ${scope}
Read AGENTS.md, PROJECT-ROADMAP.md, docs/STATUS.md and the applicable handoffs.
Preserve existing work. Verify the next eligible bounded task, update status and
handoff with evidence, then continue within this scope. Respect dependencies,
decision gates and exclusions. Do not ask whether to continue after each task.
If work remains but this turn must end, record the next concrete action.
If the entire authorized scope is verified, end your final response with a line
containing exactly AUTOCONTINUE_DONE.
If no eligible work can proceed safely, document the blockers and end your final
response with a line containing exactly AUTOCONTINUE_BLOCKED.
Never claim unperformed verification. These markers apply to the entire run,
not merely one completed task. Do not emit either marker inside a code block.`;
}

// Only this export is a plugin entry point. Tests live outside plugins/.
export const AutoContinuePlugin = async ({ client, directory }, options = {}) => {
  const limits = { ...DEFAULTS, ...options };
  for (const key of Object.keys(DEFAULTS)) {
    if (!Number.isSafeInteger(limits[key]) || limits[key] < 1) {
      throw new Error(`auto-continue: ${key} must be a positive integer`);
    }
  }
  const runs = new Map();

  async function log(message) {
    try {
      const result = await client.app.log({ body: { service: "auto-continue", level: "info", message } });
      if (result.error) throw new Error(JSON.stringify(result.error));
    } catch (error) {
      console.error(`[auto-continue] ${message}; logging failed: ${error}`);
    }
  }

  function stop(id, reason) {
    const run = runs.get(id);
    if (!run) return;
    clearTimeout(run.timer);
    runs.delete(id);
    void log(`${id}: stopped (${reason}); ${run.count} automatic follow-ups`);
  }

  async function request(method, id, extra = {}) {
    const result = await method({ path: { id }, query: { directory }, ...extra });
    if (result.error) throw new Error(JSON.stringify(result.error));
    return result.data;
  }

  async function continueRun(id, run) {
    if (runs.get(id) !== run || run.checking) return;
    run.checking = true;
    try {
      if (Date.now() - run.started >= limits.maxDurationMs || run.count >= limits.maxContinuations) {
        stop(id, "run limit reached");
        return;
      }
      const messages = await request(client.session.messages.bind(client.session), id, {
        query: { directory, limit: 20 },
      });
      if (!Array.isArray(messages)) throw new Error("Missing session messages");
      const latest = messages.at(-1);
      if (runs.get(id) !== run || !run.idle) return;
      // Never revive an aborted, incomplete, compacting, or tool-only turn.
      if (!latest || latest.info.role !== "assistant" || latest.info.summary) return;
      if (latest.info.error) {
        stop(id, "assistant error or interruption");
        return;
      }
      if (!latest.info.time?.completed || latest.info.finish !== "stop") return;
      if (latest.info.id === run.lastMessage) return;
      const text = textOf(latest.parts).trim();
      const marker = text.split(/\r?\n/).at(-1);
      if (marker === "AUTOCONTINUE_DONE" || marker === "AUTOCONTINUE_BLOCKED") {
        stop(id, marker);
        return;
      }
      run.repeats = text === run.lastText ? run.repeats + 1 : 1;
      run.lastText = text;
      if (!text || run.repeats >= 3) {
        stop(id, "empty or repeated response");
        return;
      }
      // User input or an error may have disabled the run during the read.
      if (runs.get(id) !== run || !run.idle) return;
      run.lastMessage = latest.info.id;
      run.count++;
      run.expected = `Automatic follow-up ${run.count}/${limits.maxContinuations}.\n${instructions(run.scope)}`;
      await request(client.session.promptAsync.bind(client.session), id, {
        body: {
          agent: run.agent,
          model: run.model,
          parts: [{ type: "text", text: run.expected }],
        },
      });
    } catch (error) {
      if (runs.get(id) === run) stop(id, `continuation failed: ${error}`);
    } finally {
      run.checking = false;
    }
  }

  return {
    config: async (config) => {
      config.command ??= {};
      config.command.autocontinue = {
        description: "Start a bounded autonomous run: /autocontinue <scope>",
        agent: "build",
        template: "Start an autonomous development run for: $ARGUMENTS",
      };
      config.command.autostop = {
        description: "Disable automatic follow-ups for this session",
        template: "Automatic follow-ups are disabled. Briefly report current progress and stop.",
      };
    },

    "command.execute.before": async (input, output) => {
      if (input.command === "autostop") {
        stop(input.sessionID, "user requested stop");
        return;
      }
      if (input.command !== "autocontinue") return;
      stop(input.sessionID, "restarting run");
      const scope = input.arguments.trim();
      if (!scope) throw new Error("Usage: /autocontinue <scope>, for example Projects 1–4");
      const session = await request(client.session.get.bind(client.session), input.sessionID);
      if (!session || session.parentID) throw new Error("Auto-continue requires a top-level session");
      const prompt = instructions(scope);
      output.parts.splice(0, output.parts.length, { type: "text", text: prompt });
      runs.set(input.sessionID, {
        scope, started: Date.now(), count: 0, repeats: 0,
        expected: prompt, idle: false, checking: false,
      });
      await log(`${input.sessionID}: enabled, at most ${limits.maxContinuations} automatic follow-ups`);
    },

    "chat.message": async (input, output) => {
      const run = runs.get(input.sessionID);
      if (!run) return;
      if (textOf(output.parts) !== run.expected) {
        stop(input.sessionID, "new user input");
        return;
      }
      run.expected = undefined;
      run.agent = output.message.agent ?? input.agent ?? "build";
      run.model = output.message.model ?? input.model;
      run.idle = false;
      clearTimeout(run.timer);
    },

    event: async ({ event }) => {
      const props = event.properties ?? {};
      const id = props.sessionID ?? props.info?.sessionID ?? props.info?.id;
      if (event.type === "session.error" && !id) {
        for (const key of runs.keys()) stop(key, "server error");
        return;
      }
      const run = runs.get(id);
      if (!run) return;
      if (["session.error", "session.deleted", "permission.asked", "permission.updated", "question.asked"].includes(event.type)
          || (event.type === "message.updated" && props.info?.error)) {
        stop(id, event.type);
        return;
      }
      if (event.type === "session.status" && props.status?.type !== "idle") {
        run.idle = false;
        clearTimeout(run.timer);
        return;
      }
      if (event.type !== "session.idle" && !(event.type === "session.status" && props.status?.type === "idle")) return;
      run.idle = true;
      clearTimeout(run.timer);
      // Coalesce legacy session.idle and session.status notifications.
      run.timer = setTimeout(() => { void continueRun(id, run); }, limits.delayMs);
      run.timer.unref?.();
    },

    dispose: async () => {
      for (const id of runs.keys()) stop(id, "plugin shutdown");
    },
  };
};
