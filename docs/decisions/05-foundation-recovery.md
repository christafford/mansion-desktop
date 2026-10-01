# Decision 05: reopen product gates and recover the foundation

Date: 2026-09-29. Status: accepted documentation/work-order correction;
foundation product acceptance remains pending P4-T18.

## Superseded work order (2026-10-01)

Decision 06 authorizes Godot integration and independently eligible visual
work in Project 21. Its live-terminal gate replaces P4-T18/P4-T28 as the current
development gate. This record does not prohibit that migration or require its
old recovery schedule first. Evidence and lifetime obligations remain valid;
unperformed checks remain unperformed.

## Context

The archive has useful synthetic protocol/render/input tests and basic room
geometry, but no recorded real-application visual trial. The historical room
gate permitted expansion without that evidence. Accelerated-client testing
uses GPU readback and shm, while EGL lookup conclusions need a proper re-probe.
Source review identifies resource lifetime, commit semantics, and format/input
mapping paths to audit; these leads require runtime reproduction.

## Decision

1. Preserve the implementation and historical evidence; supersede Decision 04's
   permission to continue and Decision 03's definitive capability conclusion.
2. Run foundation recovery P4-T06–P4-T18, then presentable room P4-T20–P4-T28,
   before Project 5 feature work. New numeric IDs remain compatible with the
   existing continuation parser; top-level checklist entries are required.
3. Keep C++20, Meson, EGL/GLES and the current direct libwayland-server stack
   while conducting bounded native-host/GPU/wlroots experiments. Decision 02
   remains the implemented stack record. Experiments authorize investigation,
   not wholesale migration. Decide the next integration step from evidence.
4. Real application mode/host-window behavior is mandatory foundation evidence.
   A research blocker cannot waive the terminal trial. A tested shm-only path
   may explicitly support the next bounded phase, without claiming GPU import
   or general desktop compatibility; P2-T05 remains open until import works.
5. Accept implementation/tests separately from personal visual observation.
   Keep human tasks unmarked until a person supplies the evidence. Gate tasks
   are non-human review tasks that depend on those observations, so automation
   must stop as blocked when they are unavailable.

## Acceptance record (to complete during P4-T18)

- Outcome: pending; no permission to proceed yet.
- Source revision/date, tested machine, host backend, driver and terminal: pending.
- Automated and sanitizer commands/results: pending.
- Real terminal flat/room input, resize, close, host-focus and exit evidence: pending.
- Native-host and GPU-import findings/limits: pending.
- Stack choice and any bounded follow-up/migration plan: pending.
- Remaining blockers and the exact scope authorized next: pending.

## Consequences

No implementation capability becomes complete when these documentation patches
are applied. Reopened task P2-T05 can remain blocked while explicitly approved
shm-only work proceeds. Unavailable GUI/hardware does not justify fake evidence
or unrelated feature work. Record the blocker and continue independent eligible
tasks in the authorized range; stop when none remain.
