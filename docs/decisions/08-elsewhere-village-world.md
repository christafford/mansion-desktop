# Decision 08 — Elsewhere village world (2026-10-09)

**Status:** Accepted by owner for direction; implementation pending.

## Context

The existing Godot study proved useful 3D navigation, movable/rotatable
objects, and live application panels that can be left and revisited within a
session. Connected rooms and corridors do not provide the desired delight,
exploration, or memorable geographic identity. We are changing the physical
world while retaining the existing integration approach.

## Decision

- Product name is **Elsewhere**. P21-T67 completed the technical rename; retain
  its tested runtime identifiers and paths. This decision changes the world,
  not the naming baseline. See [handoff 38](../handoffs/38-elsewhere-rename.md).
- Start with a *single compact village square* in a beautiful, explorable
  alpine/temperate setting. One landmark, differentiated exteriors, a covered
  workspace, vegetation, paths, and convincing distant boundaries. No whole
  village or functional building interiors yet.
- Retain Godot renderer, C++ Wayland compositor and GDExtension bridge, current
  launcher, input/application modes, object manipulation, and nested/headless
  development/test paths.
- Decouple world-specific geometry from desktop services before replacing the
  active study world. Keep stable application/artifact identities independent
  of scene nodes, physical room paths, runtime handles, and PIDs.
- Remove old study geometry from the **active** scene only after safe runtime
  extraction and smoke tests. Preserve historical implementation or archive it
  in a reversible manner until affected tests and references are migrated.
- Visual acceptance requires inspecting actual Godot render captures.
  Runtime integration must be tested with real applications. Missing GUI access
  is an explicit blocker, not evidence of success.
- Task creatures, extra destinations, weather, NPCs, teleportation redesign,
  and persistence across restarts remain outside this milestone.
- Owner controls token usage: only explicitly requested single P22 task; no
  autonomous continuation to the next task.

## Consequences and guardrails

Project 21 remains history/evidence but is not the active world build plan.
P22-T01 must document actual dependencies in `world/scripts/study.gd` and
extract conservatively; avoid a speculative rewrite. P22-T02 establishes a
minimal outdoor replacement and must not claim application integration is
fully verified before P22-T06. Each task records checks and precise blockers.
The first acceptance gate is **visual delight** after P22-T05; the second is
**live desktop usability** after P22-T09 and owner review at P22-T10.
