# Godot reality audit and executable work order

Date: 2026-10-02. Source revision audited: `3dfaa26`.
Assignment: reconcile status/tasks for the owner's Qwen/OpenCode autocontinue run.
This is documentation work; no Godot, bridge or asset implementation was completed
by this audit. No C++ baseline was rerun in this documentation task.

## Next action

Start **P21-T00** in [TASKS.md](../TASKS.md). Use the existing editor and successful
CLI below, verify provenance/API configuration, and run a fresh baseline. Do not
spend the next turn rewriting this plan or rebuilding Godot without a failure.
Then repair scaffold/validation in T01 and proceed by the numbered task actions.
The recommended scope is `/autocontinue P21-T00 through P21-T19`.

Default order prioritizes the visual room through T09, then the bridge through
T19. T10 is independently eligible after T00 if an art task is genuinely blocked.
Follow Depends lines; reopened prerequisites block dependent completion.
[STATUS.md](../STATUS.md) is now concise current truth; older status was moved to
[an explicitly historical archive](10-status-before-godot-audit.md).

## Why tasks were reopened

| Task | Previously claimed | Actual evidence / missing acceptance |
| --- | --- | --- |
| T00 | Pinned verified toolchain | Local executable/bindings exist, but old provenance/checksum/release/build claims were unsubstantiated; renderer naming was wrong; fresh baseline/configuration needed |
| T01 | Scaffold complete | Placeholder files exist; no recorded inspected Godot window/control trial; launcher/validator need correction |
| T02 | Curated set complete | Prose candidate list exists; no reviewable preview evidence or authoritative complete file manifest |
| T03 | All assets fetched | 22 files cached, but six glTF models reference 30 absent files; offline/failure-path acceptance missing |
| T10 | Core independent of GL | `compositor.cpp` includes GLES/display and calls renderer helpers; `input.cpp` couples seat and camera; Meson has no independent core-library target |
| T11 (already open) | Only blocked on GUI registration | `.gdextension` resource/class registration missing; init callbacks are empty; deployed library lists unresolved Elsewhere functions; no Godot-driven socket/fixture proof |

The executable the owner tested remains the C++/EGL renderer. Earlier fixes in
`a5ba64a`, `99fcedb`, `3dfaa26` repaired it; they did not implement Godot. Owner
feedback confirms those fixes, not the Project 21 product gates.

## Checks actually run during this audit

### Editor identity and runtime

- `file tools/Godot_v4.7.2-stable_linux.x86_64`: executable ELF x86-64.
- `tools/Godot_v4.7.2-stable_linux.x86_64 --version`: exit 0,
  `4.7.2.stable.official.ed1daf0bf`.
- `sha256sum tools/Godot_v4.7.2-stable_linux.x86_64`:
  `8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e`.
  This is a local fingerprint, not independently verified publisher provenance.
- `git -C tools/godot-cpp rev-parse HEAD`:
  `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`; describe: `10.0.0-stable`.
  `gdextension/extension_api-4-7.json` identifies engine API 4.7.0 single precision.

Audit isolation: `mkdtemp` under `/tmp`, copy only `world/project.godot`,
`world/scenes/` and `world/scripts/`; omit assets/native extension/generated
`.godot` directories. No owner files were overwritten. This isolates scaffold
checks; it cannot establish asset imports or extension loadability.

Commands below used that temporary copy in place of `COPY`:

```sh
timeout -k 2s 30s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path COPY --editor --import
timeout -k 2s 15s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path COPY --quit-after 3
timeout -k 2s 15s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --path COPY --rendering-method gl_compatibility --quit-after 3
```

First command: SIGABRT, `ERROR: Parameter "singleton" is null`,
`editor/editor_node.cpp:6618`. Headless runtime: exit 0. GUI runtime: exit 0;
OpenGL 4.6, Mesa 26.2.3-arch1.1, AMD Custom GPU 0405. Audio library/device warnings
ended in dummy-driver fallback. No image inspection or physical input trial.
Logs were recorded at `/tmp/elsewhere-godot-audit-jxx6pmxz/{import,run}.log`.

Follow-up with the correct minimal import invocation:

```sh
timeout -k 2s 30s tools/Godot_v4.7.2-stable_linux.x86_64 \
  --headless --path COPY --import
```

**Exit 0 on both the first copy and a fresh copy with no `.godot` cache.**
Fresh-copy log: `/tmp/elsewhere-godot-clean-audit-zxm7i69d/import.log`.
Use this invocation; do not invent a persistent missing-editor/display blocker.
The cause of the first abort was not established; successful follow-ups do not
prove all scaffold behavior, toolchain provenance or extension compatibility.
These `/tmp` logs are diagnostic artifacts, not durable acceptance screenshots;
the exact result excerpts and commands above preserve the audit finding.

### Asset dependencies

Offline inspection (no new downloads) found:

| Cached JSON glTF | Missing external buffers/images |
| --- | ---: |
| `book_encyclopedia_set_01_glb.gltf` | 7 |
| `desk_lamp_arm_01_glb.gltf` | 4 |
| `dining_chair_02_glb.gltf` | 4 |
| `metal_office_desk_glb.gltf` | 4 |
| `potted_plant_02_glb.gltf` | 7 |
| `wooden_bookshelf_worn_glb.gltf` | 4 |

Reproduce from repository root:

```sh
python3 - <<'PY'
from pathlib import Path
import json
for path in sorted(Path('world/assets/cache').glob('*.gltf')):
    model = json.loads(path.read_text())
    refs = [item['uri'] for kind in ('buffers', 'images')
            for item in model.get(kind, [])
            if 'uri' in item and not item['uri'].startswith('data:')]
    missing = [uri for uri in refs if not (path.parent / uri).is_file()]
    print(path.name, 'references:', len(refs), 'missing:', len(missing))
PY
```

Typical missing entries: `metal_office_desk.bin` and
`textures/metal_office_desk_nor_gl_2k.jpg`. Existing MD5 constants were not
independently verified in this audit; do not call them official validated hashes.
The fetcher also requests API metadata before deciding that cached content is
usable; a repeat online fetch is not proof of offline behavior.

### Core/extension source inspection

- `src/compositor.cpp` includes `<GLES2/gl2.h>` and `display.h`, and calls
  `ensure_renderer_surface` / `destroy_renderer_surface`.
- `meson.build` builds the combined legacy executable, not a core library.
- `elsewhere_extension.cpp` init/deinit callbacks have no registration logic.
  A C++ wrapper class declared in that file is not registered with Godot.
- `world/addons/elsewhere_godot/` has a `.so` and `extension.toml`, no
  `.gdextension` resource. No bridge use exists in the scene script.
- `nm -D --undefined-only world/addons/elsewhere_godot/libelsewhere_godot.so | c++filt`
  lists unresolved `create_compositor`, `create_seat`, `create_xdg_shell`,
  `launch_app` and matching teardown functions. No runtime load pass was run.

See [TOOLCHAIN.md](../TOOLCHAIN.md) for official renderer/extension references.
Do not edit `user://project.godot` or wait for a GUI settings action. T11 requires
proper linking, a loadable resource, registered methods and a fixture client.

### Documentation/plugin validation

`node --test .opencode/tests/*.test.js`: 62/62 pass in this audit.
Validate the actual task document with the exported `parseScope` and
`unfinishedTasks` functions, verify all 20 tasks T00–T19 are open, all 36 P21 IDs
remain unique, all dependency targets exist, no cycle is introduced, and T34
remains human. Check relative links and `git diff --check`. The plugin itself,
model/provider and runtime code are unchanged. No live Qwen run was performed.

## Per-task evidence template for continuation

Copy this structure into the current task's handoff when recording new work;
replace every placeholder with facts. Do not pre-fill a pass result.

```text
Task: P21-Txx
Source revision and local changes:
Implemented behavior and files:
Command:
Exit code and relevant log excerpt:
Expected result / actual result:
Evidence paths (rendered image when visual):
Observer: automated / agent image inspection / owner
Acceptance: passed / unfinished / blocked
Next exact edit or command if unfinished:
Blocker: fresh failure plus why safe local recovery is unavailable
```

A task with an unfinished mandatory line stays unchecked. If blocked, name the
next independently eligible task. Stop the recommended run at T19 only after
the furnished room and live-terminal interaction pass together; do not start
later roadmap features merely to avoid finishing a difficult current task.
