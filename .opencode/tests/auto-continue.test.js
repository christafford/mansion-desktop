import test from 'node:test';
import assert from 'node:assert/strict';
import { setTimeout as sleep } from 'node:timers/promises';
import { AutoContinuePlugin } from '../plugins/auto-continue.js';

async function harness(t, options = {}) {
  const prompts = [], logs = [];
  let messages = [], hooks;
  const model = { providerID: 'test', modelID: 'test-model' };
  const client = {
    app: { log: async ({ body }) => { logs.push(body.message); return {}; } },
    session: {
      get: async () => ({ data: { id: 'main' } }),
      messages: async () => ({ data: messages }),
      promptAsync: async (args) => {
        prompts.push(args);
        await hooks['chat.message']({ sessionID: args.path.id }, {
          message: { agent: args.body.agent, model: args.body.model }, parts: args.body.parts,
        });
        return {};
      },
    },
  };
  hooks = await AutoContinuePlugin({ client, directory: '/project' }, { delayMs: 5, ...options });
  t.after(() => hooks.dispose());
  const event = (type, properties = { sessionID: 'main' }) => hooks.event({ event: { type, properties } });
  const user = (text) => hooks['chat.message']({ sessionID: 'main' }, {
    message: { agent: 'build', model }, parts: [{ type: 'text', text }],
  });
  const start = async () => {
    const output = { parts: [] };
    await hooks['command.execute.before']({ command: 'autocontinue', sessionID: 'main', arguments: 'Projects 1–4' }, output);
    await user(output.parts[0].text);
  };
  const answer = (id = 'answer-1', text = 'Finished task one. Next: task two.', info = {}) => {
    messages = [{ info: { id, role: 'assistant', finish: 'stop', time: { completed: 1 }, ...info },
      parts: [{ type: 'text', text }] }];
  };
  const idle = async () => { await event('session.idle'); await sleep(25); };
  return { hooks, client, prompts, logs, model, event, user, start, answer, idle };
}

test('registers commands without removing existing commands', async (t) => {
  const h = await harness(t);
  const config = { command: { existing: { template: 'hello' } } };
  await h.hooks.config(config);
  assert.equal(config.command.existing.template, 'hello');
  assert.equal(config.command.autocontinue.agent, 'build');
  assert.ok(config.command.autostop);
});

test('opt-in only; scoped follow-ups retain directory, agent and model', async (t) => {
  const h = await harness(t);
  h.answer();
  await h.idle();
  assert.equal(h.prompts.length, 0);
  await h.start();
  await h.idle();
  assert.equal(h.prompts.length, 1);
  assert.equal(h.prompts[0].path.id, 'main');
  assert.equal(h.prompts[0].query.directory, '/project');
  assert.equal(h.prompts[0].body.agent, 'build');
  assert.deepEqual(h.prompts[0].body.model, h.model);
  assert.match(h.prompts[0].body.parts[0].text, /Projects 1–4/);
  h.answer('answer-2', 'Task two verified; next task three.');
  await h.idle();
  assert.equal(h.prompts.length, 2);
});

test('duplicate idle events and repeated old message produce only one follow-up', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await Promise.all([h.event('session.idle'), h.event('session.status', { sessionID: 'main', status: { type: 'idle' } })]);
  await sleep(25);
  await h.idle();
  assert.equal(h.prompts.length, 1);
});

for (const marker of ['AUTOCONTINUE_DONE', 'AUTOCONTINUE_BLOCKED']) {
  test(`stops on final ${marker} line`, async (t) => {
    const h = await harness(t);
    await h.start(); h.answer('a', `Evidence and status.\n${marker}\n`);
    await h.idle(); h.answer('b'); await h.idle();
    assert.equal(h.prompts.length, 0);
    assert.ok(h.logs.some((s) => s.includes(marker)));
  });
}

for (const type of ['session.error', 'session.deleted', 'permission.asked', 'question.asked']) {
  test(`stops on ${type}`, async (t) => {
    const h = await harness(t);
    await h.start(); h.answer();
    await h.event('session.idle');
    await h.event(type); await sleep(25); await h.idle();
    assert.equal(h.prompts.length, 0);
  });
}

test('new user input cancels a pending continuation', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await h.event('session.idle'); await h.user('Stop and explain what changed.');
  await sleep(25); await h.idle();
  assert.equal(h.prompts.length, 0);
});

test('autostop cancels a pending continuation', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer(); await h.event('session.idle');
  await h.hooks['command.execute.before']({ command: 'autostop', sessionID: 'main' }, { parts: [] });
  await sleep(25); await h.idle();
  assert.equal(h.prompts.length, 0);
});

test('user input while message fetch is in flight prevents submission', async (t) => {
  const h = await harness(t);
  let release, entered;
  const pending = new Promise((resolve) => { release = resolve; });
  const reading = new Promise((resolve) => { entered = resolve; });
  h.client.session.messages = async () => { entered(); return pending; };
  await h.start(); await h.event('session.idle');
  // Keep the event loop alive because plugin timers are intentionally unref'ed.
  await Promise.all([reading, sleep(15)]);
  await h.user('Pause.');
  release({ data: [] });
  await sleep(10);
  assert.equal(h.prompts.length, 0);
});

test('errors returned by SDK stop instead of retrying forever', async (t) => {
  const h = await harness(t);
  let attempts = 0;
  h.client.session.promptAsync = async () => { attempts++; return { error: { message: 'offline' } }; };
  await h.start(); h.answer(); await h.idle(); await h.idle();
  assert.equal(attempts, 1);
  assert.ok(h.logs.some((s) => s.includes('offline')));
});

test('continuation count is bounded', async (t) => {
  const h = await harness(t, { maxContinuations: 2 });
  await h.start();
  for (let i = 0; i < 4; i++) { h.answer(`answer-${i}`, `Task ${i} done`); await h.idle(); }
  assert.equal(h.prompts.length, 2);
});

test('elapsed time prevents further follow-ups', async (t) => {
  const h = await harness(t, { maxDurationMs: 1 });
  await h.start(); h.answer(); await h.idle();
  assert.equal(h.prompts.length, 0);
});

test('three identical responses stop the loop', async (t) => {
  const h = await harness(t);
  await h.start();
  for (let i = 0; i < 4; i++) { h.answer(`answer-${i}`, 'Nothing changed.'); await h.idle(); }
  assert.equal(h.prompts.length, 2);
});

test('ignores other sessions and child sessions cannot opt in', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer();
  await h.event('session.idle', { sessionID: 'child' }); await sleep(25);
  assert.equal(h.prompts.length, 0);
  h.client.session.get = async () => ({ data: { id: 'child', parentID: 'main' } });
  await assert.rejects(h.hooks['command.execute.before']({ command: 'autocontinue', sessionID: 'child', arguments: 'everything' }, { parts: [] }), /top-level/);
});

test('requires a scope', async (t) => {
  const h = await harness(t);
  await assert.rejects(h.hooks['command.execute.before']({ command: 'autocontinue', sessionID: 'main', arguments: ' ' }, { parts: [] }), /Usage/);
});

for (const info of [{ error: { name: 'MessageAbortedError' } }, { summary: true }, { finish: 'tool-calls' }, { time: {} }]) {
  test(`does not continue non-final replies: ${JSON.stringify(info)}`, async (t) => {
    const h = await harness(t);
    await h.start(); h.answer('a', 'Some text', info); await h.idle();
    assert.equal(h.prompts.length, 0);
  });
}

test('busy status cancels timer; next idle can resume', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer(); await h.event('session.idle');
  await h.event('session.status', { sessionID: 'main', status: { type: 'busy' } });
  await sleep(25); assert.equal(h.prompts.length, 0);
  await h.idle(); assert.equal(h.prompts.length, 1);
});

test('shutdown clears pending timers', async (t) => {
  const h = await harness(t);
  await h.start(); h.answer(); await h.event('session.idle');
  await h.hooks.dispose(); await sleep(25);
  assert.equal(h.prompts.length, 0);
});
