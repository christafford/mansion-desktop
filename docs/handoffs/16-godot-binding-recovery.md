# Standard C++ binding recovery — 2026-10-03

P21-T37 establishes the native binding boundary independently of the unfinished
Wayland adapter. It does not implement compositor services or live pixels.

## Implementation

- `src/godot/binding_probe.cpp`: ordinary godot-cpp RefCounted class,
  `_bind_methods`, scene-level registration, initializer and terminator via
  `GDExtensionBinding::InitObject`. One native method returns 42.
- `tests/godot-binding/`: real `.gdextension` resource and script assertions.
- `tools/check-godot-binding.sh`: builds compatible generated headers/library in
  `build-godot-probe/`, rejects unresolved symbols at link, imports a fresh copy,
  and verifies three separate process lifetimes with 100 native instances each.
  Script checks fail if the class is missing or the native return is wrong;
  logs also reject script errors and engine-reported leaked instances.

The official [initialization example](https://docs.godotengine.org/en/latest/tutorials/scripting/cpp/gdextension_cpp_example.html)
was checked against the local bindings. No manually allocated Godot ABI structs,
raw pixel addresses, legacy renderer linkage or guessed function signatures.

## Failures investigated

1. Reusing the previous godot-cpp build failed because generated headers included
   a missing old interface header. Sources were newer/inconsistent with that
   generated build. A separate generation/build directory fixed that boundary;
   the user's existing build was preserved.
2. The minimal build profile initially omitted OS, which this checkout's
   `print_string.cpp` includes. Added OS to the profile; dependency sources were
   not patched and compiler errors were not suppressed.
3. A first editor import crashed after scanning, while runtime native calls
   passed. Reproduced with fresh copies and even an empty project. GDB using the
   already available development engine traced a null `DocTools` pointer in
   `EditorHelp::_gen_extensions_docs` during `Main::cleanup`. Ten unpaced frames
   failed too. `--import --quit-after 180 --max-fps 60` passed on two fresh
   projects; the final check itself always stages a fresh cache. This remains a
   documented timing mitigation, not a claim that the engine bug was repaired.

## Verification and limits

- Local engine and bindings identities are in [TOOLCHAIN.md](../TOOLCHAIN.md).
- Binding build/load/call/release checks pass; three process lifetimes / 300
  native calls. No error or engine-reported instance-leak logs. No sanitizer or
  heap-leak proof is claimed for this probe.
- C++ build and 31/31 Meson tests pass; 62/62 existing Node plugin tests pass.
- Scene validator passes and still rejects the intentionally invalid script.
- Old C++ compositor, adapter attempts and tracked binary remain untouched and
  outside these commits. The crashing addon remains ignored by the study.

Next is a bounded compositor lifecycle connection: one owned server, private
socket, nonblocking server-event-loop pump, actual fixture connection and clean
shutdown. Do not add texture/input methods before that check passes. P21-T10,
T11 and T12 retain their full acceptance and are not checked by this probe.
