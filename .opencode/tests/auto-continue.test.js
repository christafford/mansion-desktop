import test from 'node:test';
import assert from 'node:assert/strict';
import { setTimeout as sleep } from 'node:timers/promises';
import plugin, { createPlugin, DEFAULTS, DONE, BLOCKED, resolveLimits, assistantText, instructions } from '../plugins/auto-continue.js';

// Mock of the OpenCode V2 Promise plugin context: only the parts the plugin uses.
async function harness(t, { options = {}, fingerprints, events = true } = {}) {
  const prompts = [], commands = new Map(), hooks = new Map();
  let messages = [], session = { id: 'main', time: { created: 1, updated: 1 } };
  let sink;
  const queue = [];
  const nextEvent = () => new Promise((resolve) => { sink = resolve; });
  const ctx = {
    options: { delayMs: 5, pollMs: 0, ...options },
    location: { directory: '/project' },
    session: {
      get: async ({ sessionID }) => (sessionID === 'child' ? { id: 'child', parentID: 'main' } : session),
      context: async () => ({ data: messages }),
      prompt: async (input) => {
        prompts.push(input);
        // OpenCode runs the prompt hook for every admitted prompt, including ours.
        for (const cb of hooks.get('prompt') ?? []) await cb({ sessionID: input.sessionID, prompt: { text: input.text } });
        return { id: `inbox-${prompts.length}` };
      },
      hook: async (name, cb) => { hooks.set(name, [...(hooks.get(name) ?? []), cb]); return { dispose: async () => {} }; },
    },
    command: { transform: async (fn) => fn({ add: (c) => commands.set(c.name, c) }) },
    event: events ? {
      subscribe: ({ signal }) => ({
        async *[Symbol.asyncIterator]() {
          while (!signal.aborted) {
            if (queue.length) { yield queue.shift(); continue; }
            const next = await Promise.race([nextEvent(), new Promise((r) => signal.addEventListener('abort', () => r(null), { once: true }))]);
            if (next) yield next;
          }
        },
      }),
    } : undefined,
  };
  let prints = fingerprints ?? [];
  let printIndex = 0;
  const fingerprint = async () => prints.length ? prints[Math.min(printIndex++, prints.length - 1)] : `print-${printIndex++}`;
  const cleanup = await createPlugin({ fingerprint }).setup(ctx);
  t.after(() => cleanup());
  const event = async (type, data = { sessionID: 'main' }) => {
    const ev = { type, data };
    if (sink) { const s = sink; sink = null; s(ev); } else queue.push(ev);
    await sleep(2);
  };
  let id = 0;
  const answer = (text = 'Finished task one. Next: P1-T02', extra = {}, { idle = true, user } = {}) => {
    const created = 100 + ++id * 10;
    const userText = user ?? prompts.at(-1)?.text ?? '';
    messages = [
      { id: `u${id}`, type: 'user', text: userText, time: { created: created - 2 } },
      { id: `a${id}`, type: 'assistant', content: [{ type: 'reasoning', text: 'thinking' }, { type: 'text', text }],
        finish: 'stop', time: { created, completed: created + 1 }, ...extra },
      ...(idle ? [{ id: `i${id}`, type: 'idle', outcome: 'succeeded', time: { created: created + 2 } }] : []),
    ];
  };
  const finish = async (...args) => { answer(...args); await event('session.execution.succeeded'); await sleep(30); };
  const start = (scope = 'Projects 1–2') => commands.get('autocontinue').execute({ sessionID: 'main', prompt: { text: scope }, delivery: 'queue' });
  const user = async (text) => { for (const cb of hooks.get('prompt') ?? []) await cb({ sessionID: 'main', prompt: { text } }); };
  return { ctx, prompts, commands, event, answer, finish, start, user, setMessages: (m) => { messages = m; }, setSession: (s) => { session = s; } };
}

test('default export is a V2 plugin definition', () => {
  assert.equal(plugin.id, 'auto-continue');
  assert.equal(typeof plugin.setup, 'function');
});

test('limits: defaults, env overrides, options override env, validation', () => {
  assert.deepEqual(resolveLimits({}, {}), DEFAULTS);
  const env = { AUTOCONTINUE_MAX_CONTINUATIONS: '7', AUTOCONTINUE_MAX_HOURS: '1.5', AUTOCONTINUE_STALL_LIMIT: '0' };
  const l = resolveLimits({}, env);
  assert.equal(l.maxContinuations, 7);
  assert.equal(l.maxDurationMs, 1.5 * 3600 * 1000);
  assert.equal(l.stallLimit, 0);
  assert.equal(resolveLimits({ maxContinuations: 3 }, env).maxContinuations, 3);
  assert.throws(() => resolveLimits({ delayMs: 0 }, {}), /delayMs/);
  assert.throws(() => resolveLimits({}, { AUTOCONTINUE_MAX_HOURS: 'soon' }), /number/);
});

test('assistantText joins text parts only', () => {
  assert.equal(assistantText({ content: [{ type: 'reasoning', text: 'r' }, { type: 'text', text: 'a' }, { type: 'text', text: 'b' }] }), 'a\nb');
  assert.equal(assistantText({ parts: [{ type: 'text', text: 'v1' }] }), 'v1');
  assert.equal(assistantText({}), '');
});

test('registers both commands and requires a scope', async (t) => {
  const h = await harness(t);
  assert.ok(h.commands.has('autocontinue'));
  assert.ok(h.commands.has('autostop'));
  await assert.rejects(h.commands.get('autocontinue').execute({ sessionID: 'main', prompt: { text: ' ' } }), /Usage/);
  assert.equal(h.prompts.length, 0);
});

test('child sessions cannot opt in', async (t) => {
  const h = await harness(t);
  await assert.rejects(h.commands.get('autocontinue').execute({ sessionID: 'child', prompt: { text: 'x' } }), /top-level/);
});

test('start sends the instructions; each finished turn produces one follow-up', async (t) => {
  const h = await harness(t);
  await h.start('Projects 1–2');
  assert.equal(h.prompts.length, 1);
  assert.equal(h.prompts[0].sessionID, 'main');
  assert.equal(h.prompts[0].text, instructions('Projects 1–2'));
  assert.match(h.prompts[0].text, /Projects 1–2/);
  await h.finish();
  assert.equal(h.prompts.length, 2);
  assert.match(h.prompts[1].text, /^Automatic follow-up 1\//);
  assert.equal(h.prompts[1].delivery, 'queue');
  await h.finish('Task two verified. Next: P1-T03');
  assert.equal(h.prompts.length, 3);
});

test('opt-in only: turns in sessions without a run are ignored', async (t) => {
  const h = await harness(t);
  await h.finish();
  assert.equal(h.prompts.length, 0);
});

test('duplicate completion events produce a single follow-up', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await h.event('session.execution.succeeded');
  await h.event('session.execution.succeeded');
  await sleep(40);
  await h.event('session.execution.succeeded'); await sleep(30);
  assert.equal(h.prompts.length, 2);
});

for (const marker of [DONE, BLOCKED]) {
  test(`stops on a final ${marker} line`, async (t) => {
    const h = await harness(t);
    await h.start();
    await h.finish(`Evidence and status.\n${marker}\n`);
    await h.finish('more');
    assert.equal(h.prompts.length, 1);
  });
}

test('a marker inside the text but not on the last line does not stop', async (t) => {
  const h = await harness(t);
  await h.start();
  await h.finish(`I will write ${DONE} later.\nNext: P1-T02`);
  assert.equal(h.prompts.length, 2);
});

for (const type of ['session.execution.failed', 'session.execution.interrupted', 'session.deleted', 'permission.asked', 'form.created']) {
  test(`stops on ${type}`, async (t) => {
    const h = await harness(t);
    await h.start(); h.answer();
    await h.event(type);
    await h.event('session.execution.succeeded'); await sleep(30);
    assert.equal(h.prompts.length, 1);
  });
}

test('failed or interrupted idle outcome stops the run', async (t) => {
  const h = await harness(t);
  await h.start();
  h.answer('text');
  const { data } = await h.ctx.session.context({ sessionID: 'main' });
  h.setMessages([...data.slice(0, 2), { id: 'i', type: 'idle', outcome: 'interrupted', time: { created: 999 } }]);
  await h.event('session.execution.succeeded'); await sleep(30);
  assert.equal(h.prompts.length, 1);
});

test('new user input through the prompt hook cancels the run', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await h.user('Stop and explain what changed.');
  await h.event('session.execution.succeeded'); await sleep(30);
  assert.equal(h.prompts.length, 1);
});

test('a newer user message in the transcript cancels the run', async (t) => {
  const h = await harness(t);
  await h.start();
  h.answer('reply', {}, { user: 'Actually, pause.' });
  await h.event('session.execution.succeeded'); await sleep(30);
  assert.equal(h.prompts.length, 1);
});

test('autostop cancels and sends a report request', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await h.commands.get('autostop').execute({ sessionID: 'main', delivery: 'queue' });
  assert.equal(h.prompts.length, 2);
  assert.match(h.prompts[1].text, /disabled/);
  await h.event('session.execution.succeeded'); await sleep(30);
  assert.equal(h.prompts.length, 2);
});

test('prompt failures stop instead of retrying forever', async (t) => {
  const h = await harness(t);
  await h.start();
  let attempts = 0;
  h.ctx.session.prompt = async () => { attempts++; throw new Error('offline'); };
  await h.finish(); await h.finish();
  assert.equal(attempts, 1);
});

test('continuation count is bounded', async (t) => {
  const h = await harness(t, { options: { maxContinuations: 2 } });
  await h.start();
  for (let i = 0; i < 4; i++) await h.finish(`Task ${i} done`);
  assert.equal(h.prompts.length, 3);
});

test('elapsed time prevents further follow-ups', async (t) => {
  const h = await harness(t, { options: { maxDurationMs: 1 } });
  await h.start(); await sleep(5); await h.finish();
  assert.equal(h.prompts.length, 1);
});

test('identical final replies stop the loop', async (t) => {
  const h = await harness(t);
  await h.start();
  for (let i = 0; i < 4; i++) await h.finish('Nothing changed.');
  assert.equal(h.prompts.length, 3);
});

test('empty final reply stops the loop', async (t) => {
  const h = await harness(t);
  await h.start(); await h.finish('   ');
  assert.equal(h.prompts.length, 1);
});

test('a streak of follow-ups without repository changes stops the run', async (t) => {
  const h = await harness(t, { options: { stallLimit: 2 }, fingerprints: ['same'] });
  await h.start();
  for (let i = 0; i < 4; i++) await h.finish(`Investigating ${i}`);
  // start(1) + follow-ups while stalls < 2: first check stalls=1 -> sent, second check stalls=2 -> stop.
  assert.equal(h.prompts.length, 2);
});

test('repository changes reset the stall counter', async (t) => {
  const h = await harness(t, { options: { stallLimit: 2 } });
  await h.start();
  for (let i = 0; i < 5; i++) await h.finish(`Step ${i}`);
  assert.equal(h.prompts.length, 6);
});

for (const [label, extra, opts] of [
  ['incomplete', { time: { created: 5 } }, {}],
  ['tool-calls without idle marker', { finish: 'tool-calls' }, { idle: false }],
  ['error', { error: { type: 'ProviderError', message: 'boom' } }, {}],
]) {
  test(`does not continue non-final replies: ${label}`, async (t) => {
    const h = await harness(t);
    await h.start(); await h.finish('Some text', extra, opts);
    assert.equal(h.prompts.length, 1);
  });
}

test('tool-calls finish followed by an idle marker still counts as a completed turn', async (t) => {
  const h = await harness(t);
  await h.start(); await h.finish('Some text', { finish: 'tool-calls' });
  assert.equal(h.prompts.length, 2);
});

test('execution start cancels a pending follow-up timer', async (t) => {
  const h = await harness(t, { options: { delayMs: 40 } });
  await h.start(); h.answer();
  await h.event('session.execution.succeeded');
  await h.event('session.execution.started');
  await sleep(80);
  assert.equal(h.prompts.length, 1);
  await h.event('session.execution.succeeded'); await sleep(80);
  assert.equal(h.prompts.length, 2);
});

test('poll fallback detects idleness without events', async (t) => {
  const h = await harness(t, { events: false, options: { pollMs: 10 } });
  await h.start(); h.answer();
  h.setSession({ id: 'main', time: { created: 1, updated: 2, idle: 3 } });
  await sleep(60);
  assert.equal(h.prompts.length, 2);
});

test('cleanup cancels a pending follow-up', async (t) => {
  const prompts = [];
  const ctx = {
    options: { delayMs: 30, pollMs: 0 },
    location: { directory: '/project' },
    session: {
      get: async () => ({ id: 'main', time: { created: 1, updated: 1 } }),
      context: async () => ({ data: [
        { id: 'u', type: 'user', text: instructions('x'), time: { created: 1 } },
        { id: 'a', type: 'assistant', content: [{ type: 'text', text: 'done one' }], finish: 'stop', time: { created: 2, completed: 3 } },
        { id: 'i', type: 'idle', outcome: 'succeeded', time: { created: 4 } },
      ] }),
      prompt: async (input) => { prompts.push(input); return {}; },
      hook: async () => ({}),
    },
    command: { transform: async (fn) => fn({ add: () => {} }) },
    event: { subscribe: () => ({ async *[Symbol.asyncIterator]() {} }) },
  };
  const cleanup = await createPlugin({ fingerprint: async () => 'x' }).setup(ctx);
  // Start a run directly through the internal command path by re-adding commands.
  let autocontinue;
  ctx.command.transform = async (fn) => fn({ add: (c) => { if (c.name === 'autocontinue') autocontinue = c; } });
  const cleanup2 = await createPlugin({ fingerprint: async () => 'x' }).setup(ctx);
  await autocontinue.execute({ sessionID: 'main', prompt: { text: 'x' } });
  assert.equal(prompts.length, 1);
  cleanup(); cleanup2();
  await sleep(80);
  assert.equal(prompts.length, 1);
});
