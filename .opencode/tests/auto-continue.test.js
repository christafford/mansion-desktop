import test from 'node:test';
import assert from 'node:assert/strict';
import { setTimeout as sleep } from 'node:timers/promises';
import plugin, { createPlugin, DEFAULTS, DONE, BLOCKED, resolveLimits, assistantText, instructions, parseScope, unfinishedTasks } from '../plugins/auto-continue.js';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

// Task list used by default: everything in Projects 1–2 ticked except a human task.
const TASKS_DONE = `
- [x] **P1-T01 Harness.** (aaa)
- [x] **P1-T02 Globals.** (bbb)
- [ ] **P1-T03 (human) Terminal smoke test.** needs a person
- [x] **P2-T01 Math.** (ccc)
- [ ] **P3-T01 Modes.** later
`;
const TASKS_OPEN = TASKS_DONE.replace('- [x] **P2-T01', '- [ ] **P2-T01');

// Stable fixture: all P1–P4 tasks ticked except a human task. Used to replace
// the live TASKS.md test so the suite does not break when the real file is
// fully completed. Includes a reopened gate (P2-T05) blocked on P4-T13.
const TASKS_ALL_P1_P4_DONE = `
- [x] **P1-T01 Harness.** (aaa)
- [x] **P1-T02 Globals.** (bbb)
- [ ] **P1-T03 (human) Terminal smoke test.** needs a person
- [x] **P2-T01 Math.** (ccc)
- [ ] **P2-T05 Accelerated client experiment (reopened).** blocked on P4-T13
- [x] **P3-T01 Modes.** (ddd)
- [x] **P4-T01 Room geometry.** (eee)
- [x] **P4-T02 Monitor slot.** (fff)
- [x] **P4-T03 Milestone 1 demo.** (ggg)
- [ ] **P4-T04 (human) Real terminal.** needs a person
- [x] **P4-T05 Project 4 wrap-up.** (hhh)
`;

// Fixture that includes open P5 tasks to verify visibility.
const TASKS_WITH_P5 = `
- [x] **P1-T01 Harness.** (aaa)
- [ ] **P2-T05 Accelerated client experiment (reopened).** blocked on P4-T13
- [x] **P3-T01 Modes.** (ccc)
- [x] **P4-T01 Room geometry.** (ddd)
- [x] **P4-T05 Project 4 wrap-up.** (eee)
- [x] **P5-T00 Expand Project 5.** (fff)
- [ ] **P5-T01 Window registry.** requires P4-T28
- [ ] **P5-T02 Focus cycling.** requires P5-T01
`;

function memoryStore(initial = {}) {
  const store = { state: initial, saves: 0 };
  store.load = async () => JSON.parse(JSON.stringify(store.state));
  store.save = async (state) => { store.saves++; store.state = JSON.parse(JSON.stringify(state)); };
  return store;
}

// Mock of the OpenCode V2 Promise plugin context: only the parts the plugin uses.
async function harness(t, { options = {}, fingerprints, events = true, tasks = TASKS_DONE, store = memoryStore(), sessions } = {}) {
  const prompts = [], commands = new Map(), hooks = new Map();
  let messages = [], session = { id: 'main', time: { created: 1, updated: 1 } };
  const lookup = sessions ?? ((sessionID) => (sessionID === 'child' ? { id: 'child', parentID: 'main' } : session));
  let sink;
  const queue = [];
  const nextEvent = () => new Promise((resolve) => { sink = resolve; });
  const ctx = {
    options: { delayMs: 5, pollMs: 0, ...options },
    location: { directory: '/project' },
    session: {
      get: async ({ sessionID }) => lookup(sessionID),
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
  const readTasks = async () => { if (tasks instanceof Error) throw tasks; return tasks; };
  const make = () => createPlugin({ fingerprint, readTasks, stateStore: () => store });
  let cleanup = await make().setup(ctx);
  t.after(() => cleanup());
  // reload(): unload the plugin and set it up again against the same store, as OpenCode does on file change.
  const reload = async () => { cleanup(); cleanup = await make().setup(ctx); await sleep(10); };
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
  return { ctx, prompts, commands, event, answer, finish, start, user, store, reload, setMessages: (m) => { messages = m; }, setSession: (s) => { session = s; } };
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

// ── DONE verification against docs/TASKS.md ─────────────────────────────

test('parseScope understands project ranges and task ids', () => {
  assert.deepEqual([...parseScope('Projects 1–4 of docs/TASKS.md').projects], [1, 2, 3, 4]);
  assert.deepEqual([...parseScope('projects 2 to 3').projects], [2, 3]);
  assert.deepEqual([...parseScope('Project 7').projects], [7]);
  const ids = parseScope('Project 1 (P1-T03 to P1-T11)');
  assert.deepEqual([...ids.projects], [1]);
  assert.deepEqual(ids.taskRanges, [{ from: [1, 3], to: [1, 11] }]);
  assert.deepEqual(parseScope('P2-T05').taskRanges, [{ from: [2, 5], to: [2, 5] }]);
  assert.equal(parseScope('make it nice'), null);
  assert.equal(parseScope(''), null);
});

test('unfinishedTasks ignores ticked, human, and out-of-scope tasks', () => {
  assert.deepEqual(unfinishedTasks(TASKS_DONE, 'Projects 1–2'), []);
  assert.deepEqual(unfinishedTasks(TASKS_OPEN, 'Projects 1–2'), ['P2-T01']);
  assert.deepEqual(unfinishedTasks(TASKS_OPEN, 'Projects 1–3'), ['P2-T01', 'P3-T01']);
  assert.deepEqual(unfinishedTasks(TASKS_OPEN, 'P1-T01 to P1-T03'), []);
  assert.deepEqual(unfinishedTasks(TASKS_OPEN, 'P2-T01'), ['P2-T01']);
  assert.equal(unfinishedTasks(TASKS_OPEN, 'no range here'), null);
});

test('the repository task list is parsed: Projects 1–4 are not finished', async () => {
  // Use a stable fixture instead of the live TASKS.md so the suite does not
  // break when the real file is fully completed.
  const open = unfinishedTasks(TASKS_ALL_P1_P4_DONE, 'Projects 1–4 of docs/TASKS.md');
  assert.ok(open.length > 0, 'expected unticked tasks in the all-P1-P4-done fixture');
  assert.ok(open.every((id) => /^P[1-4]-T\d+$/.test(id)), open.join(','));
  assert.ok(!open.includes('P1-T03') && !open.includes('P4-T04'), 'human tasks must not be required');
});

test(`${DONE} with unticked tasks in scope continues and names them`, async (t) => {
  const h = await harness(t, { tasks: TASKS_OPEN });
  await h.start('Projects 1–2');
  await h.finish(`All done.\n${DONE}`);
  assert.equal(h.prompts.length, 2);
  assert.match(h.prompts[1].text, /still has unticked tasks in scope: P2-T01/);
  assert.match(h.prompts[1].text, /Autonomous run scope: Projects 1–2/);
  await h.finish('Working on P2-T01. Next: P2-T01');
  assert.equal(h.prompts.length, 3);
  assert.doesNotMatch(h.prompts[2].text, /unticked tasks/);
});

test(`${DONE} is honoured once every in-scope task is ticked`, async (t) => {
  const h = await harness(t, { tasks: TASKS_DONE });
  await h.start('Projects 1–2');
  await h.finish(`Evidence.\n${DONE}`);
  await h.finish('more');
  assert.equal(h.prompts.length, 1);
  assert.deepEqual(h.store.state, {}, 'finished run is removed from the state file');
});

test(`repeated false ${DONE} claims stop the run`, async (t) => {
  const h = await harness(t, { tasks: TASKS_OPEN, options: { falseDoneLimit: 2 } });
  await h.start('Projects 1–2');
  await h.finish(`Done!\n${DONE}`);
  assert.equal(h.prompts.length, 2);
  await h.finish(`Really done.\n${DONE}`);
  await h.finish('anything');
  assert.equal(h.prompts.length, 2);
});

test(`${DONE} is accepted when the scope names no task range`, async (t) => {
  const h = await harness(t, { tasks: TASKS_OPEN });
  await h.start('tidy the docs');
  await h.finish(`Finished.\n${DONE}`);
  await h.finish('more');
  assert.equal(h.prompts.length, 1);
});

test(`an unreadable task list stops the run instead of trusting ${DONE}`, async (t) => {
  const h = await harness(t, { tasks: Object.assign(new Error('ENOENT'), { code: 'ENOENT' }) });
  await h.start('Projects 1–2');
  await h.finish(`Finished.\n${DONE}`);
  await h.finish('more');
  assert.equal(h.prompts.length, 1);
});

test(`${BLOCKED} still stops without consulting the task list`, async (t) => {
  const h = await harness(t, { tasks: TASKS_OPEN });
  await h.start('Projects 1–2');
  await h.finish(`Blocked on hardware.\n${BLOCKED}`);
  await h.finish('more');
  assert.equal(h.prompts.length, 1);
});

// ── P5 task visibility and gate blocking ──────────────────────────────────

test('P5 tasks are visible when included in scope', async (t) => {
  const open = unfinishedTasks(TASKS_WITH_P5, 'Projects 1–5 of docs/TASKS.md');
  assert.ok(open.includes('P5-T01'), 'P5-T01 must be visible');
  assert.ok(open.includes('P5-T02'), 'P5-T02 must be visible');
  assert.ok(open.includes('P2-T05'), 'reopened P2-T05 must be visible');
  assert.ok(!open.includes('P1-T03') && !open.includes('P5-T00'), 'human and completed tasks excluded');
});

test('reopened gate task is required for DONE verification', async (t) => {
  const open = unfinishedTasks(TASKS_ALL_P1_P4_DONE, 'Projects 1–4 of docs/TASKS.md');
  assert.ok(open.includes('P2-T05'), 'reopened P2-T05 must block DONE');
  // Even though all P1 tasks are done and P4-T04 is human, P2-T05 keeps
  // the project incomplete.
  assert.ok(open.length > 0, 'reopened gate prevents false completion');
});

test('unknown scope rejects DONE verification in normal runs', async (t) => {
  // When the scope names no recognizable project or task range,
  // DONE is accepted unverified (the plugin logs this).
  // This test verifies the parsing behavior.
  assert.equal(parseScope('tidy the docs'), null);
  assert.equal(parseScope(''), null);
  assert.equal(parseScope('fix bugs'), null);
  // These should NOT parse to any project:
  const scope = parseScope('tidy the docs');
  assert.equal(scope, null, 'non-project scope returns null');
});

test('unknown scope does not trigger blocked gate behavior in normal runs', async (t) => {
  // An unknown scope cannot be verified, so DONE is accepted and the run
  // stops without checking the task list. This is the expected "fail closed"
  // behavior: the plugin stops rather than blindly trusting an unverifiable
  // scope claim.
  const h = await harness(t, { tasks: TASKS_OPEN });
  await h.start('tidy the docs');
  await h.finish(`Finished.\n${DONE}`);
  await h.finish('more');
  assert.equal(h.prompts.length, 1, 'unknown scope accepts DONE unverified');
});

test('numeric IDs are parsed correctly across all projects', async (t) => {
  assert.deepEqual([...parseScope('Projects 1–8').projects], [1, 2, 3, 4, 5, 6, 7, 8]);
  assert.deepEqual([...parseScope('P4-T06').taskRanges], [{ from: [4, 6], to: [4, 6] }]);
  assert.deepEqual([...parseScope('P4-T06 to P4-T18').taskRanges], [{ from: [4, 6], to: [4, 18] }]);
  assert.deepEqual([...parseScope('P5-T01').projects], []);
  assert.ok(parseScope('P5-T01').taskRanges.length > 0, 'single task P5 scope parsed');
});

// ── Persistence across plugin reloads ───────────────────────────────────

test('run state is saved on start and after each follow-up', async (t) => {
  const h = await harness(t);
  await h.start('Projects 1–2');
  assert.equal(h.store.state.main.scope, 'Projects 1–2');
  assert.equal(h.store.state.main.count, 0);
  await h.finish();
  assert.equal(h.store.state.main.count, 1);
  assert.equal(h.store.state.main.expected, h.prompts[1].text);
  assert.equal(h.store.state.main.timer, undefined, 'timers are not persisted');
});

test('a reload resumes the run and continues after the next finished turn', async (t) => {
  const h = await harness(t, { tasks: TASKS_OPEN });
  await h.start('Projects 1–2');
  await h.finish('Step one. Next: P2-T01');
  assert.equal(h.prompts.length, 2);
  await h.reload();
  assert.ok(h.store.state.main, 'state survives the unload');
  assert.equal(h.prompts.length, 2, 'a reload alone does not re-prompt while the turn is running');
  await h.finish('Step two. Next: P2-T01');
  assert.equal(h.prompts.length, 3);
  assert.match(h.prompts[2].text, /^Automatic follow-up 2\//, 'the follow-up counter continues');
  await h.finish(`All ticked.\n${DONE}`);
  assert.equal(h.prompts.length, 4, 'the resumed run still verifies DONE');
});

test('a reload after the turn already ended sends the pending follow-up', async (t) => {
  const h = await harness(t);
  await h.start('Projects 1–2');
  h.answer('Finished while unloaded. Next: P1-T02');
  await h.reload();
  await sleep(40);
  assert.equal(h.prompts.length, 2);
});

test('saved runs whose session is gone are dropped on load', async (t) => {
  const store = memoryStore({ ghost: { scope: 'Projects 1–2', started: 1, count: 3, expected: 'x' } });
  const h = await harness(t, { store, sessions: (id) => (id === 'ghost' ? undefined : { id, time: { created: 1 } }) });
  await sleep(10);
  assert.deepEqual(h.store.state, {});
  await h.event('session.execution.succeeded', { sessionID: 'ghost' }); await sleep(30);
  assert.equal(h.prompts.length, 0);
});

test('autostop removes the saved state', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await h.commands.get('autostop').execute({ sessionID: 'main', delivery: 'queue' });
  await sleep(10);
  assert.deepEqual(h.store.state, {});
});

test('a broken state store does not prevent starting a run', async (t) => {
  const store = { load: async () => { throw new Error('disk'); }, save: async () => { throw new Error('disk'); } };
  const h = await harness(t, { store });
  await h.start('Projects 1–2');
  await h.finish();
  assert.equal(h.prompts.length, 2);
});
