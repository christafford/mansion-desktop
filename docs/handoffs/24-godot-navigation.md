# World mouse navigation — 2026-10-04

P21-T46 is the owner's navigation request, based on `a0da90e`. Horizontal mouse
look is reversed. Right-button release eases the camera's vertical look angle
back to the existing arrival pitch (-0.08 radians) over 0.2 seconds. This changes
pitch, not player height. Yaw and held walking keys survive release. Re-grabbing
cancels the animation; Home, pointer cleanup and application-mode transitions
also cancel it so it cannot fight a later controller action.

Implementation is confined to the existing Godot player controller. No C++,
dependencies or host settings changed. Unrelated overnight edits are preserved.

## Verification

Agent-run checks, Godot `4.7.2.stable.official.ed1daf0bf`, Compatibility/OpenGL
on AMD Custom GPU 0405. `tools/check-godot-study.sh` includes import/script/runtime
validation and graphical controller/terminal regressions. The extended
`study_smoke.gd` verifies both horizontal directions, downward and upward pitch
return, yaw preservation, retained walking keys, ignored released mouse motion,
interruption on re-grab and Home reset. Injected events establish these behaviors;
physical mouse feel and human comfort are not observed.

Import/runtime, transforms (8), keyboard mapping (27), pointer mapping (302),
controller (56 textured surfaces, zero failures), color and two live terminal
sessions pass. The first full run exited 1 at the keyboard test: activation
initially succeeded, then application mode stopped accepting input. This is
consistent with host focus interference, but its precise trigger was not
instrumented. The unchanged keyboard test reran separately with exit 0,
9 repeated characters, 11,163 pixel comparisons and zero failures. Pointer and
resize tests then ran sequentially with exit 0 and zero failures: real selection,
scrolling and all four terminal sizes pass (8,573 resize pixel comparisons).
No test was weakened or production focus policy bypassed. This is successful
coverage across these runs, not a claim of one uninterrupted full-suite pass.

Logs are retained locally in `.tools/recovery/t46/`, including the failed first
run and successful follow-ups. Reproduce the follow-ups with the local Godot
binary, `--path world --audio-driver Dummy --max-fps 60 --script`, and each of
`res://tests/terminal_input.gd`, `res://tests/terminal_pointer.gd`, and
`res://tests/terminal_resize.gd`. The full command is
`tools/check-godot-study.sh`. Markdown links, unique task IDs, task-parser
compatibility and the task's diff checks pass. C++/plugin tests were not rerun
for this GDScript-only change; historical results remain attributed separately.

## Next

P21-T45 remains the next recovery task: targeted client close and explicit
terminal relaunch. Follow [handoff 23](23-godot-terminal-resize.md) and
[TASKS.md](../TASKS.md). T46 does not satisfy broader product or human gates.
