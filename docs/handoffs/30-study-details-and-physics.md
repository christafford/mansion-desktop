# Owner-requested room details and physical objects

2026-10-05, starting revision `35d6b3d`. This bounded request covers T53–T55;
it does not authorize a broader roadmap run or complete a human gate.

## T53: ceiling fittings and wall decoration

Two original 1.38m enamel twin-tube fittings provide shadowed downlight, with
asynchronous small ballast variation (measured multiplier 0.94655–0.99976 over
60 simulated seconds). Tube emission follows the same variation. The original
unshadowed room fill was removed, daylight reduced and ambient retained. No
renderer, dependency or application-color changes. The wall details are two
original botanical SVG prints, a clock showing opening time, and a noticeboard.
See [original asset provenance](../../world/art/README.md).

Verified this session: `SDL_JOYSTICK_LINUX_CLASSIC=1 tools/validate-godot-project.sh`,
graphical `res://tests/study_details.gd` and `res://tests/screen_color.gd`.
All passed; 226198 sampled pixels changed in the ceiling-shadow comparison,
and all 11 synthetic client-color patches retained their expected RGB values.
The new scene test is part of `tools/check-godot-study.sh`.

Agent-observed GPU captures at 1280×800, Godot 4.7.2 official ed1daf0bf,
Compatibility OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405:
[arrival](../evidence/t53/arrival.png), [desk](../evidence/t53/desk.png),
[reverse](../evidence/t53/reverse.png), [ceiling](../evidence/t53/ceiling.png),
[shadows on](../evidence/t53/shadows.png) and
[ceiling shadows disabled](../evidence/t53/shadows-disabled.png).
Cameras are fixed in the scene test; flicker is frozen only for comparison.
Initial captures exposed excessive floor light and cropped BoxMesh artwork UVs;
energy was reduced and artwork moved to full-UV quads, then captured again.
The images show actual geometry, downward chair/desk shadows, framed artwork,
noticeboard and live terminal. These are agent observations, not owner comfort
or performance acceptance; remaining coarse shadow/texture sampling and broad
finished-room gates are not claimed resolved.

Implementation uses Godot's documented
[SpotLight3D](https://docs.godotengine.org/en/stable/classes/class_spotlight3d.html)
direction and shadow support; no new asset or runtime dependency.

## Continuation

T54 pushable chair, then T55 panel gravity and full regression suite.
