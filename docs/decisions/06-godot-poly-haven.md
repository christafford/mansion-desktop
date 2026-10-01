# Decision 06: Godot world frontend and Poly Haven study

Date: 2026-10-01. Status: authorized implementation direction; product acceptance pending.

The owner has authorized using Godot and pre-built assets, and extended autonomous
work without a time deadline. The existing flat polygons are a prototype, not the
visual target. Project 21 is the current executable work order. This decision
supersedes Decision 05's engine prohibition and serial foundation-before-art
schedule, and the old Project 4 renderer plan. It does not claim any implementation
or acceptance has occurred. Historical decisions and checkmarks remain records.

## Architecture and scope

1. Godot owns the visible nested host window, main loop, 3D scene, materials,
   lighting, camera, collisions, picking, UI and animation. Implement its project
   under `world/`. Keep C++20/libwayland-server and the tested compositor core;
   expose a small C++ GDExtension adapter using a compatible pinned `godot-cpp`.
   Preserve a standalone headless compositor and existing protocol tests.
2. Extract the core incrementally from the combined renderer/host code. Godot
   drives a nonblocking compositor pump on the owning thread. It must not run a
   second blocking event loop or reach into Wayland resources from arbitrary
   engine threads. Shutdown, callback ownership and cancellation are explicit.
3. The initial bridge copies supported `wl_shm` content into an owned frame
   snapshot. Godot creates/updates an ImageTexture. Copies, stride, supported
   formats, alpha, damage, orientation, scale, transform and generations must be
   tested. Never retain a client buffer after releasing it. This is a CPU bridge,
   not zero-copy, DMA-BUF import or general accelerated application support.
4. Godot input comes only from its focused host window. Map engine keycodes to
   Wayland seat/XKB keycodes explicitly; do not feed Godot codes straight to the
   seat. Pointer mapping and focus transitions share the presentation transform.
   No `/dev/input` grabbing, permission changes or background global key capture.
5. Stable world/artifact IDs, placements and resources remain independent of
   scene-node paths, PIDs, Wayland pointers and renderer objects. Furniture and
   monitor screen slots use parent-relative transforms; a desk move carries its
   monitor and application panel with it. World lighting does not shade app text.
6. Build one exceptional study first: real furniture, distinct floor/wall/ceiling
   materials, thoughtful light, a monitor with live content, reliable controls,
   and fast access. Prefer Poly Haven PBR models, textures and HDRIs. Other assets
   may fill genuine catalog gaps with recorded provenance. A second connected
   room, persistent artifacts, search and reminder creatures follow the live
   terminal integration gate; a vast empty mansion is not an acceptance target.
7. Visual authoring and core extraction are independently eligible. They meet at
   the live-terminal gate. GPU import, wlroots migration, DRM/logind session work,
   portals and broad desktop compatibility are later programs, not prerequisites
   for this study. Keep those unimplemented tasks unchecked.

## Dependencies, execution and evidence

Godot, compatible godot-cpp, and offline Blender conversion where needed are
authorized dependencies. Pin actual releases/commits, source URLs, checksums,
licenses and build commands in `docs/TOOLCHAIN.md` during P21-T00. Use official
documentation matching those versions. Repository-local tools/download caches
are allowed. Do not install system packages, use sudo, replace host services,
change input permissions or install a native desktop session. Missing system
dependencies are precise blockers; continue independent work.

No fixed wall-clock deadline or follow-up-count cap applies by default. Quality
and finite task acceptance decide completion. Unlimited time does not mean
unlimited download volume, RAM, GPU memory, retry loops or unsupported claims.
Work in coherent commits, preserve user changes, and checkpoint when context is
low. Repair reproducible failures and try evidence-backed alternatives. Once no
eligible task can progress, record blockers and stop. Human acceptance is the
final review; it does not block independent art authoring earlier in the plan.

Use task-specific checks: core tests for C++, extension load and lifecycle tests
for the bridge, import checks for assets, actual rendered captures for visuals,
and real native clients for integration. Headless logic tests cannot prove a
rendered scene looks good. Agent-observed GUI evidence is labelled as such;
human tasks require the owner's observation. Product performance is measured on
the target Steam Deck, not inferred from a headless compositor FPS counter.

## Primary references

- [GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/what_is_gdextension.html)
- [3D import formats](https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/importing_3d_scenes/available_formats.html)
- [ImageTexture](https://docs.godotengine.org/en/stable/classes/class_imagetexture.html)
- [Rendering methods](https://docs.godotengine.org/en/stable/tutorials/rendering/renderers.html)
- [Poly Haven assets license](https://polyhaven.com/license)
- [Poly Haven API terms](https://github.com/Poly-Haven/Public-API/blob/master/ToS.md)

The stable documentation URLs can change. Record the versions actually used;
do not invent API names or assume an OpenGL texture handle is a Godot Vulkan RID.
