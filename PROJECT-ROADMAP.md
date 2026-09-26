# Mansion Desktop: assessment and executable project roadmap

This plan accompanies [the concept](mansion-desktop-concept.md). It proposes an implementation sequence; it does not claim that any prototype has been built or tested.

## Assessment

The concept is coherent and technically plausible. Its strongest decisions are keeping ordinary files and applications, storing spatial placement separately, distinguishing world input from application input, acknowledging the limits of session restoration, and developing as a nested compositor first.

The difficult part is building a reliable compositor with a spatial shell. Drawing a mansion is comparatively straightforward. A terminal demo will establish feasibility for one client; it will not establish compatibility with browsers, editors, dialogs, accelerated clients, or an entire desktop session.

There are two independent questions to answer:

1. **Engineering:** Can live applications appear in the world and become fully usable without input, rendering, or lifecycle failures?
2. **Usefulness:** Does spatial placement help someone resume real work enough to justify the navigation and maintenance cost?

The first four projects answer the initial engineering question. Projects 5–8 create the first useful nested workspace and test the second question. Stop to assess the evidence at both boundaries before increasing scope.

## Changes I recommend to the concept

- **Treat fullscreen as a presentation mode initially.** Show the client at full size inside Mansion's host window. Do not require direct scanout, changing the physical display mode, or a client fullscreen state transition just to focus it. Define client-requested fullscreen behavior separately.
- **Make the rendering choice an experiment.** Start by evaluating Rust + Smithay and its GLES/custom-rendering path. Do not commit to Vulkan/wgpu until live-buffer integration has been demonstrated. Smithay documents both nested backends and a GLES renderer; its GlowRenderer exposes custom GL rendering with explicit context-state precautions. This makes a shared GL path a reasonable first experiment, not a proven implementation. [Backend documentation](https://smithay.github.io/smithay/smithay/backend/index.html), [GlowRenderer documentation](https://smithay.github.io/smithay/smithay/backend/renderer/glow/struct.GlowRenderer.html).
- **Separate resources, artifacts, and live windows.** A file or launch recipe is a resource; its representation in a room is an artifact; a connected client window is temporary runtime state. One application can have several windows, and one resource may eventually have several spatial references. Never persist Wayland object IDs or process IDs as durable identity.
- **Avoid promising exact launch-to-window matching.** Application IDs and titles are hints, not unique instance identifiers. Applications may reuse existing processes. Use launch tracking and supported activation mechanisms, with an explicit manual assignment fallback.
- **Treat saving as a pending operation.** A file chooser selects a destination; the application writes the file afterward. Placement should initially be provisional, with cancellation, failure, overwrite, and filename handling. The portal response is not evidence that bytes were saved. [FileChooser API](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html).
- **Introduce fast access early.** A task list, search, teleportation, and reduced-motion navigation are needed to evaluate the product fairly. Do not wait for a full mansion.
- **Prefer deliberate placement over automatic clutter.** Indexing a directory should not spawn thousands of physical objects. Search results become artifacts when the user places them.
- **Keep broad desktop replacement as a later program.** Locking, display hotplug, suspend/resume, input methods, screen sharing, and recovery deserve their own acceptance criteria.

## Working structure

Use one repository and initially one executable, with internal boundaries for compositor, renderer/world, shell, and persistence. Separate projects here mean separately reviewable deliverables, not separate repositories or services.

Start with simple geometry, fixed placement slots, one host window, one supported machine/GPU configuration, and native Wayland clients. SQLite is a suitable starting point for placement metadata. Defer a physics engine until basic placement actually needs it.

Each numbered project is a separately verifiable unit of work. A fresh AI session is optional; an explicitly authorized autonomous roadmap run may complete several projects in sequence. Some graphics and protocol projects will require several sessions; use the same project brief with the previous handoff rather than expanding its scope. No credible session-count estimate exists before the first rendering experiment.

During an autonomous roadmap run, verify each completed task, update the status and handoff, and immediately begin the next eligible task within the authorized milestone or project range. Preserve dependencies and decision gates. Record blocked requirements without claiming they passed, and continue with independent eligible work where possible. Follow the stopping conditions in AGENTS.md; finishing one project or writing a handoff is not itself a reason to stop the run.

For every project, leave:

- Reproducible build/run commands and required dependencies.
- A working demonstration or a documented blocker with evidence.
- Relevant automated checks plus a short manual acceptance procedure for visible/input behavior.
- A handoff recording changed files, decisions, validation performed, limitations, and the next task.

Suggested locations: `docs/decisions/`, `docs/handoffs/NN.md`, and `docs/STATUS.md`. Create these during Project 1. Record the pinned dependency versions and actual target hardware rather than assuming library examples match the installed version.

## Projects 1–4: prove the core interaction

### 1. Nested compositor foundation

**Depends on:** nothing.

**Deliver:** a reproducible minimal compositor launched inside the existing desktop, plus a helper that launches a native Wayland terminal on its private display socket. Inspect the host environment, evaluate an appropriate Smithay example/backend, pin dependencies, and record the stack decision. Keep child-client environment changes local to the launcher.

**Done when:** a terminal displays in 2D, accepts typing and pointer input, resizes, and can close without killing Mansion. The host desktop remains usable after Mansion exits. Document what happens to connected clients when Mansion stops.

**Exclude:** 3D, persistence, XWayland, portals, native session installation.

**Session task:** “Implement Project 1 of PROJECT-ROADMAP.md. Establish the smallest working nested compositor and document its exact launch and terminal smoke-test commands.”

### 2. Live application surface in 3D

**Depends on:** 1.

**Deliver:** the existing terminal rendered on a perspective-transformed panel, with a movable camera. Define the compositor-to-world boundary: surface/window handle, logical dimensions, buffer scale/transform, lifetime, damage, and rendering responsibility. Start with one supported buffer path, then test an accelerated native client before accepting the stack.

**Done when:** visibly changing terminal output updates on the panel; resizing, closing, and reopening do not leave invalid textures; an accelerated client either works or produces a precisely recorded integration blocker. Document buffer ownership and frame scheduling. Measure frame time and any CPU copying on the target machine.

**Exclude:** detailed room art, input directly on the angled panel, a second graphics API unless the first experiment demonstrates a need.

**Session task:** “Implement Project 2. Prove real client-buffer rendering in perspective, not screenshot capture or a fake terminal. Preserve the working 2D path for comparison.”

### 3. World/application input state machine

**Depends on:** 2.

**Deliver:** explicit world and application modes; selecting the panel shows its application at full size inside Mansion. A configurable reserved shortcut returns to world mode. Handle focus loss, held keys/buttons, cursor visibility, client exit, and window resize during transitions.

**Done when:** repeated enter/type/click/scroll/leave cycles work without input leaking into the world or leaving stuck keys in the client. Losing host-window focus releases capture appropriately. The app remains running when returning to the world. Document host shortcuts Mansion cannot capture.

**Exclude:** embedded application interaction and pointer-lock-dependent applications.

**Session task:** “Implement Project 3 with an explicit input state machine and focused transition checks. Preserve application lifetime when leaving application mode.”

### 4. One-room end-to-end prototype

**Depends on:** 3.

**Deliver:** a simple room, desk, monitor, camera movement, targeting, selection, and immediate reduced-motion access to the monitor. Use basic collision bounds rather than a general physics simulation.

**Done when:** the nine-step Milestone 1 demonstration in the concept works from a clean launch, including returning to the still-running terminal. Record a repeatable demonstration and known limitations.

**Exclude:** visual polish and additional rooms.

**Session task:** “Implement Project 4 using the real compositor surface from Projects 1–3. Complete and document the concept's Milestone 1 acceptance sequence.”

**Decision gate A:** Continue only when the real surface and input path works. If it fails, revise the renderer/compositor integration before adding features or scenery.

## Projects 5–8: build a useful persistent workspace

### 5. Multiple windows and application lifecycle

**Depends on:** 4.

**Deliver:** runtime window registry, several artifacts, launch/close/reassign actions, and policies for additional top-level windows, transient dialogs, popups, and subsurfaces. Compose related surfaces correctly in focused view and define their world-preview behavior. Include manual artifact assignment when automatic association is ambiguous.

**Done when:** a native terminal, browser, and editor can coexist and switch focus; menus and dialogs stay associated with the correct application; closing one window does not remove another. Record exact tested applications and versions.

**Exclude:** universal compatibility claims and XWayland.

**Session task:** “Implement Project 5. Expand the single-client prototype into a tested multiwindow workspace, including popup/dialog behavior and ambiguous window assignment.”

### 6. Durable artifact model and placement

**Depends on:** 5.

**Deliver:** a versioned SQLite schema separating resources, artifacts, and temporary live-window bindings; stable artifact UUIDs; slot-based movement; transactional saves; reset/export support. Define local versus room-relative transforms. Enforce valid container membership without cycles.

**Done when:** move three artifacts, restart Mansion, and find their placeholders at the saved positions. Schema migration and interrupted-write behavior are checked. No stale surface pointer or process ID is treated as a restorable application.

**Exclude:** automatic application relaunch and arbitrary physics placement.

**Session task:** “Implement Project 6. Persist artifact identity and placement independently of live client state, with restart and database integrity checks.”

### 7. Launch recipes and controlled restoration

**Depends on:** 6.

**Deliver:** explicit launch recipes with application identity, argument arrays, working directory, and optional document URI; user-triggered restoration; launch state/error feedback; duplicate suppression and manual reassociation. Distinguish placeholders, launching, connected, and failed states.

**Done when:** after restarting Mansion, activating a saved artifact recreates a supported terminal or document application at its previous location. Missing executables and ambiguous instances produce recoverable states. Document which application-owned session details survive.

**Exclude:** restoring unsaved application memory and automatically rerunning arbitrary shell history.

**Session task:** “Implement Project 7. Add explicit, recoverable relaunch from durable artifacts, including failure and duplicate-instance handling.”

### 8. Fast access and usability trial

**Depends on:** 7.

**Deliver:** searchable artifact/task list, keyboard selection, direct focus, teleportation, motion settings, and an obvious way back to a known location. Add a conventional overview that can recover misplaced items.

**Done when:** every placed artifact can be reached without walking. Run a real workflow with editor, terminal, and browser; record time/actions to resume work, navigation friction, and perceived usefulness compared with the user's current desktop. Results may be mixed; report them honestly.

**Exclude:** generalized content indexing and elaborate search trails.

**Session task:** “Implement Project 8. Make all existing work reachable with keyboard search and evaluate whether the spatial layout helps a real workflow.”

**Decision gate B:** This is the first useful nested MVP. Improve it until it is pleasant to use before building the rest of the mansion. If users consistently bypass spatial navigation, investigate whether rooms still help organization or whether the metaphor needs revision.

## Projects 9–14: files and desktop integration

### 9. File resources and external change handling

**Depends on:** 6 and 8.

**Deliver:** explicit file import, selected-directory indexing, MIME metadata, and watching/reconciliation. Keep search inventory separate from placed artifacts. Define policies for rename, deletion, replacement, symlinks, and unavailable mounts. Provide manual relinking when identity is uncertain; paths and inode numbers alone are not universal durable identity.

**Done when:** moving an artifact never moves its file; a watched rename updates the reference when detectable; deletion or an unavailable mount leaves a recoverable placeholder. Restart reconciliation catches changes missed while Mansion was stopped. Indexing a large folder does not create a room full of objects or block rendering.

**Session task:** “Implement Project 9. Add file-backed artifacts and a bounded index with explicit external-change and missing-resource behavior.”

### 10. Application associations and radial chooser

**Depends on:** 7 and 9.

**Deliver:** desktop-entry/MIME association lookup, default-open action, keyboard-accessible radial chooser, and association preferences. Use established launch facilities where practical; do not interpret desktop-entry Exec fields as arbitrary shell commands. Account for existing-instance activation and manual window reassignment.

**Done when:** representative text, image, and PDF files open in compatible installed applications; paths with spaces work; absent handlers have a fallback; the resulting window can be associated with the selected artifact. Do not offer applications merely because they are installed if they do not claim suitable file support.

**Session task:** “Implement Project 10. Open real file artifacts through Linux application associations, with a radial chooser and keyboard equivalent.”

### 11. Clipboard and drag-and-drop

**Depends on:** 5 and 10.

**Deliver:** native-client clipboard transfer and standard text/file drag-and-drop between supported clients and shell targets. Define whether a dropped file creates a reference or performs an explicitly requested file operation. Specify host/nested clipboard bridging separately.

**Done when:** copy/paste between two nested applications and dropping a file onto a supported target work, including cancellation and source-app exit. The implementation does not silently imply host-desktop bridging.

**Session task:** “Implement Project 11. Add internal clipboard and drag-and-drop with explicit file-reference semantics and a documented host boundary.”

### 12. XWayland compatibility

**Depends on:** 5 and 11.

**Deliver:** XWayland lifecycle and integration with the existing artifact, focus, popup, and clipboard models.

**Done when:** a verified X11 client works alongside a native client; focus, menus/dialogs, resizing, and cross-client clipboard work. XWayland failure leaves the shell recoverable and its affected artifacts identifiable.

**Session task:** “Implement Project 12. Integrate XWayland using the existing window model and record a small, explicit compatibility matrix.”

### 13. Spatial open/save portal

**Depends on:** 9 and 10; stable focus/dialog handling from 5.

**Deliver:** a custom FileChooser backend with conventional filename/path controls plus optional spatial placement. Keep pending requests separate from saved artifacts. Handle cancellation, overwrites, application exit, and failed writes. Establish a destination-folder policy instead of deriving filesystem paths implicitly from furniture names.

**Done when:** a portal-using test application can open and save, cancel without a false saved artifact, and recover from a failed write. Verify a sandboxed application separately. Run development portal services with an isolated session/bus configuration so testing does not replace the host desktop's services. Document non-portal fallback behavior.

**Session task:** “Implement Project 13. Build and test an isolated spatial FileChooser backend, treating placement as provisional until the file actually appears.”

### 14. Notifications and basic shell controls

**Depends on:** 8; portal/service isolation conventions from 13 where applicable.

**Deliver:** notifications with history and activation back to an artifact, a small settings interface, and basic audio controls through an existing audio service. Define nested-session service ownership explicitly. Do not build a new audio server.

**Done when:** notifications are discoverable without camera movement, activation finds the right artifact where association is available, and controls work while an app is focused. Host notification services remain unaffected during nested testing.

**Session task:** “Implement Project 14. Add accessible notifications and essential settings/audio controls with explicit nested-session service ownership.”

## Projects 15–16: expand and harden the world

### 15. Multiple rooms and visual assets

**Depends on:** 8 and 9.

**Deliver:** data-driven rooms and furniture, room-relative placements, doors/teleports, thumbnails, and background loading. Preserve stable room/artifact IDs when changing assets. Add simple customization only after navigation works.

**Done when:** two project rooms survive restart; moving between them does not relaunch clients; all their artifacts remain searchable; asset/thumbnail work does not stall input. Record asset provenance and licenses.

**Session task:** “Implement Project 15. Extend the proven one-room workspace into two persistent project rooms with data-driven geometry and fast access.”

### 16. Rendering performance and compatibility hardening

**Depends on:** 12 and 15 for the full matrix; investigate performance regressions earlier as they occur.

**Deliver:** measured frame timing, idle CPU/GPU use, visible/hidden-client scheduling, GPU-memory accounting, and targeted buffer/synchronization fixes. Avoid per-frame CPU readback in the normal accelerated path. Test scaling and resizing, and expand the tested driver matrix as hardware becomes available.

**Done when:** agreed frame-time and memory targets are met on named hardware with a fixed workload; hidden apps resume correctly; client/renderer failures have useful diagnostics. Publish what was actually tested, especially for different GPU vendors.

**Session task:** “Implement Project 16. Measure the current workspace, agree on explicit budgets in the handoff, and fix demonstrated rendering/lifecycle bottlenecks without speculative rewrites.”

## Projects 17–20: become a native desktop session

These remain separate follow-on projects. A mature nested application can be a worthwhile deliverable even if this phase is deferred.

| Project | Dependencies | Deliverable and acceptance boundary |
| --- | --- | --- |
| **17. Native display/input backend** | 16 | Run on one physical display using DRM/KMS, seat/session management, and libinput; verify VT switching, device access loss/reacquisition, and return to another session. Preserve nested mode for development. |
| **18. Session safety and lifecycle** | 17 | Login-session entry, secure locking, logout, suspend/resume, and failure recovery. Demonstrate that the lock prevents application input/content exposure and that logout/crash behavior is documented. A drawn overlay alone is not a lock. |
| **19. Displays and input breadth** | 18 | Multiple monitors, hotplug, scaling, keyboard layouts, input methods, and accessibility work with explicit target-user acceptance tests. Split into display and input sessions when needed. |
| **20. Sharing, packaging, and release qualification** | 19 and 13 | Screenshot/screencast portal integration with consent, reproducible packages for one chosen distribution, install/uninstall instructions, diagnostics, and a tested support matrix. Run real daily workflows before describing it as a daily-driver desktop. |

For these sessions, use the project number and acceptance boundary above with the general session template below. Expand each brief from observed requirements before implementing it; do not ask one AI session to “finish desktop integration.”

## Optional later projects

- Direct input into angled in-world surfaces, with correct popup, scale, and coordinate handling. Full-size application mode should remain available.
- Freeform placement and richer physics, only if fixed slots prove limiting.
- Shared/synchronized layouts with machine-specific resource mapping and conflict rules.
- Remote-machine portals, VR, advanced architecture editing, and elaborate graphics.

None of these is necessary to prove the core concept.

## Copyable prompt for a single project

```text
Read mansion-desktop-concept.md and PROJECT-ROADMAP.md.
Read docs/STATUS.md, relevant docs/decisions/, and the latest applicable
docs/handoffs/ if they exist. Inspect the current repository before changing it.

Work on Project [NUMBER]: [TITLE].
Verify that its prerequisite demonstrations work. If a prerequisite is broken,
report and fix the smallest relevant regression before building on it.

Implement this project's stated scope and acceptance criteria. Preserve existing
working demonstrations. Treat the recommended stack as provisional until the
recorded integration experiment establishes it; respect later recorded decisions.
Do not add deferred features or replace the architecture without evidence.

Run relevant checks and the documented demonstration where this environment
supports it. Clearly distinguish verified behavior from checks requiring my GUI
or hardware. Do not claim visual/input tests passed without running them.

Update docs/STATUS.md and docs/handoffs/[NUMBER].md with what changed, exact run
commands, validation evidence, limitations, unresolved decisions, and the next
bounded task. If incomplete, leave a precise continuation point for a fresh session.
```

## Copyable prompt for an autonomous roadmap run

Replace [FIRST] and [LAST] with the authorized project range before using this prompt.

```text
Read AGENTS.md, mansion-desktop-concept.md, and PROJECT-ROADMAP.md.
Read docs/STATUS.md, relevant docs/decisions/, and the latest applicable
docs/handoffs/. Inspect the current repository and preserve uncommitted work.
Reconcile stale status claims with the implementation and verification evidence.

Run an autonomous development sequence through Projects [FIRST]–[LAST].
Select the next eligible bounded task in that range. Verify prerequisites,
implement the stated scope, and run relevant checks and demonstrations.
Respect recorded stack decisions, dependencies, exclusions, and decision gates.

After each task, update docs/STATUS.md and the applicable handoff with changes,
exact validation commands and results, limitations, and the next bounded task.
Immediately begin the next eligible task without asking whether to continue.
Do not end the run merely because one task or numbered project is complete.

If verification requires unavailable hardware, services, or human observation,
record the blocker and continue with independent eligible work. Do not claim
unperformed checks passed or bypass an unverified prerequisite or decision gate.

Stop when the authorized range is verified, a limit I specified is reached,
I ask you to stop, or no eligible work remains that can be performed safely.
On stopping, report verified results and any remaining blockers precisely.
```

Start with **Project 1**, then make **Project 2** the main technical go/no-go experiment. The first product target is **Project 8**, a useful persistent nested workspace; the complete native desktop is a substantially larger undertaking.
