# Decision 04 — Milestone 1 gate (Project 4)

Date: 2026-09-27
Related tasks: P4-T01, P4-T02, P4-T03, P4-T05

## Question

Is the current room-mode implementation ready to serve as the foundation
for a multi-room persistent workspace? Specifically:

1. Does the room rendering pipeline work end-to-end?
2. Is camera collision reliable?
3. Does the panel-to-monitor mapping work?
4. Can the full user journey (walk → select → type → return) be scripted?
5. Are there any showstopper bugs?

## Findings

### Room rendering

The room is rendered as a 10×7×10 box with coloured flat-shaded meshes:
floor (brown), walls/ceiling (grey), desk (dark brown), monitor frame (dark grey).
The room draws with depth test; the panel draws on top without depth test.
Visual verification: `room-render` test checks floor colour (61472c±40) at
bottom centre and wall colour (7f7f8c±40) at top centre pixel. **Passes.**

### Camera collision

AABB collision clamps camera to x∈[-4.5,4.5], y∈[0.5,6.5], z∈[-4.5,4.5]
when `room_mode` is true. Collision is **not** active in non-room modes.
Verification: `room-collision` test walks forward (W key) until the camera
hits the wall at z≈-4.5 and stops. **Passes.**

### Panel-to-monitor mapping

In room mode, the panel is positioned at (0, 3, -5) — the monitor frame's
location. The panel texture (the client's surface) appears on the monitor.
This was verified in the milestone1 test which shows the client connecting
and its surface being rendered on the panel/monitor. **Passes.**

### Full user journey (milestone1)

The `tests/milestone1.sh` script runs the complete nine-step journey:
1. Start compositor in room-camera mode
2. Walk forward (W key) toward the monitor
3. Client is mapped on the monitor (panel at monitor position)
4. Approach the monitor
5. Select panel (Enter key) → Application mode
6. Type keys (client reports key 2, 3, 4)
7. Return to world mode (F12 key)
8. Verify client still connected

All checks pass. **Passes.**

### Teleport

Key T (evdev 20) teleports the camera to (0, 3, 0) with yaw=0, pitch=0,
facing the monitor. Works in both World and Application modes. **Passes.**

### Texture swizzle

Fixed a pre-existing bug where the EGL path swizzled RGBA data incorrectly
for RGBA input (client writes ARGB8888 → RGBA on little-endian). The fix
swizzles (data[1], data[0], data[2], data[3]) in EGL mode and uploads
directly in non-EGL mode. All 24 tests pass with this fix. **Passes.**

## Test results

```
 24/24 p4 - mansion-desktop:milestone1  OK
 24/24 p4 - mansion-desktop:teleport     OK
 24/24 p4 - mansion-desktop:room-render  OK
 24/24 p4 - mansion-desktop:room-collision OK
24/24 total — all pass
```

## Conclusion

**Gate A: Continue to Project 5.**

The room-mode prototype is stable and the full user journey works
end-to-end. The foundation is ready for multi-room expansion (Project 5+).

### Strengths
- Clean separation: room rendering, panel rendering, input modes
- Camera collision is reliable and only active in room mode
- Teleport provides reduced-motion access
- All 24 tests pass (including regression tests from earlier projects)
- The milestone1 script demonstrates the full user journey

### Open items
- Single room only — no room switching
- No door/portal geometry
- No persistent room data (rooms are hardcoded)
- P4-T04 (real terminal demo) is a human task
- Windowed mode (X11) has not been observed

### Recommended next steps
1. Project 5: expand to multiple rooms with room switching.
2. Add data-driven room definitions (JSON or similar).
3. Implement room-relative surface placement (not hardcoded coordinates).
4. Add doors/portals between rooms.
