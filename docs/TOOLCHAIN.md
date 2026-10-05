# Godot toolchain — measured local state, 2026-10-03

Decision 06 authorizes these repository-local dependencies. No system packages
or host services were changed. Reproducible upstream acquisition remains part of
P21-T00; a local file name or version string is not upstream checksum provenance.

| Component | Measured identity |
| --- | --- |
| Engine | `tools/Godot_v4.7.2-stable_linux.x86_64` |
| Engine `--version` | `4.7.2.stable.official.ed1daf0bf` |
| Local engine SHA-256 | `8d106cbe6144c2dc7e881d61d2429c1a8a76e6b22ef48bd5e48dcf934953f71e` |
| godot-cpp checkout | `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5` |
| Probe target API | 4.7, single precision, Linux x86_64, template_debug |
| Observed scene renderer | Compatibility / OpenGL 4.6, Mesa 26.2.4-arch1.1, AMD Custom GPU 0405 |

The scene's actual captures are in [frontend evidence](evidence/godot-recovery/).
Renderer identity is not a frame-time benchmark. Compatibility is OpenGL, not
Vulkan. Complete glTF packages import directly; Blender is not required here.

## Supported checks

The application launcher also uses installed Python 3.14.7, PyGObject 3.56.3,
GIO/GioUnix 2.88.3 and GTK 3.24.52 introspection. GTK resolves icons in a helper
process; Godot renders the UI. KDE/Plasma/KRunner is not required. These measured
versions and the vendored MIT radial asset are covered by
[Decision 07](decisions/07-application-launcher.md). Missing introspection
dependencies produce a launcher error; no automatic system installation occurs.
The radial asset's upstream revision, local patch and hashes are recorded in
[UPSTREAM.md](../world/addons/advanced_radial_menu/UPSTREAM.md).

```sh
python3 world/tools/prepare_study_assets.py
tools/validate-godot-project.sh
tools/run-godot.sh --audio-driver Dummy
tools/check-godot-binding.sh
```

The scene validator checks import and runtime logs, rejects errors even with
exit status zero, and retains unique per-run logs. Dummy audio avoids an
irrelevant unavailable-audio-device dependency; graphical checks still use the
real display and GPU.

The binding check builds a small standard godot-cpp extension using CMake under
`build-godot-probe/`, with `tests/godot-binding/build_profile.json`. It includes
`OS` because this godot-cpp checkout's print helper requires it. It exercises a
fresh project cache and repeated native instantiation/calls/release and shutdown.
The old `tools/godot-cpp/build/` has inconsistent generated headers and is not
used or overwritten. Do not mix those headers with a different binding library.

## Runtime startup joystick workaround (2026-10-05)

The owner's pre-render startup SIGSEGV maps to the bundled SDL Linux joystick
device sort. `tools/run-godot.sh` now defaults `SDL_JOYSTICK_LINUX_CLASSIC=1`,
preserving an explicit override. GDB confirms this avoids the affected evdev
comparison branch; five graphical starts and real Terminal/Vim interaction pass.
This is a bounded engine workaround, not a proven repair of the intermittent
underlying fault. Direct engine invocations do not inherit the launcher default.
See [startup handoff](handoffs/27-godot-startup.md) for addresses and limitations.

## Local editor import shutdown defect

Bare `--import` reproducibly crashes on fresh small projects, including a project
without an extension. GDB with the existing local development engine located a
null `DocTools` access from `EditorHelp::_gen_extensions_docs`, called through a
deferred message during `Main::cleanup`. The engine's asynchronous cached-doc
loader can finish after documentation teardown. This is distinct from the old
Mansion addon's crashing class registration.

The tested mitigation is:

```sh
tools/Godot_v4.7.2-stable_linux.x86_64 --headless --audio-driver Dummy \
  --path PROJECT --import --quit-after 180 --max-fps 60
```

Pacing gives the documentation worker time to finish; ten unpaced frames did
not fix the race. Two independent fresh-copy checks passed with pacing before
adopting it in the scripts. All nonzero exits and error logs still fail. This is
a measured workaround for the installed engine, not an upstream engine fix or
a universal timing guarantee. Runtime binding calls do not require the editor.
See [binding handoff](handoffs/16-godot-binding-recovery.md).

## Remaining T00 work

Record verified upstream acquisition URLs and checksums, build/pin bootstrap,
and a fresh-machine installation procedure. Local identities, successful tests
and the working import configuration do not by themselves complete that gate.
Use the standard initialization pattern in the
[Godot C++ example](https://docs.godotengine.org/en/latest/tutorials/scripting/cpp/gdextension_cpp_example.html),
checked against the actual local headers and runtime. The latest documentation
may describe different versions; passing the actual load/call check is required.
