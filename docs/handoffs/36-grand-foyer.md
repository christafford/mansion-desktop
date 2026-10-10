# Grand foyer and spiral staircase

Owner request 2026-10-09, from `817aff3`, P21-T64. This completes the destination
built after T63's paired-room merges. The former observatory is replaced by a
20 × 24m foyer with a 13m elliptical barrel vault and a gallery at 5.6m. The end
hallway door remains approach-opening: its low, narrow passage reveals the tall
stone interior. No additional rooms are added beyond it.

Original geometry supplies the ashlar vault and ribs, columns, arched recesses,
opal glazing, compass floor inlay, rose window, teal banners and candle-ring
chandelier. The central walnut/stone stair has forty 0.14m risers over 450 degrees,
with turned brass balusters, continuous timber handrails and a bridge onto the
surrounding gallery. Console tables and plants use existing movable assemblies;
fixed structure stays fixed. Existing walnut and plant assets are reused, with
no new dependency, external asset download or bitmap generation.

The character uses a continuous layer-8 stair guide with an underside and
collision exceptions for the two detailed tread/waist bodies. The player mask
changes from 5 to 13; application and furniture masks remain unchanged, so they
still land on the actual flat treads. This avoids a reproduced lower-tread snag
on descent without changing ordinary movement controls. Both stair rails and
gallery rails have collision guards. The front gallery extends to the entrance
wall, leaving no unguarded gap behind it. Full foyer placement bounds allow
applications on either level; collision still rejects intersections with the
vault, rails and other structure.

Turned balusters share a MultiMesh. Exact wall-sized occluders hide the previous
furnished wings when they are behind foyer walls, preserving the entrance
opening. The roof casts double-sided shadows, fixing sunlight leaking through
its inward-facing surface. The two foyer spotlights use reversed shadow-face
culling to remove self-shadow
striping on columns while retaining architectural shadows. Small local lights
fade their distant shadows/illumination; nearby room and hallway lighting is
retained, and the larger foyer lights have range-scaled distances. This avoids
redrawing hidden rooms' shadow maps from the far end of the foyer. Directional
sun settings remain unchanged. See Godot's
[Light3D settings](https://docs.godotengine.org/en/stable/classes/class_light3d.html).
These changes retain visible geometry and nearby shadows.
The old `elsewhere.observatory` destination is intentionally replaced by the stable
`elsewhere.foyer` world ID; live application identities remain independent. Session
placements are not restored across restarts.

## Verification

Verified 2026-10-09 by the agent, `817aff3` plus T64, Godot 4.7.2 ed1daf0bf,
Compatibility/OpenGL 4.6, Mesa 26.2.4, AMD Custom GPU 0405, private GPU Weston
1280×800, existing 4× MSAA and 60 FPS cap. Agent-inspected engine captures:
[entrance](../evidence/t64/entrance.png), [spiral](../evidence/t64/spiral.png),
[upper gallery](../evidence/t64/upper-gallery.png),
[return view](../evidence/t64/return.png). These are rendered game views, not
concept art. The gallery application capture and ground-floor terminal views
remain in `.tools/grand-foyer-test` and `.tools/elsewhere-rooms-test`.

The full study script passed all 20 Godot trials and 7 Python tests in one run
(`.tools/t64/full-check.log`, `/tmp/elsewhere-study-check.81lJuR`). After final
light-distance/shadow-face tuning, import/runtime/native-helper validation
passed again (`.tools/t64/final-import.log`, `/tmp/elsewhere-godot-check.1kFwOV`),
as did `study_details`, `hallway`, `elsewhere_rooms`, `grand_foyer`, and `screen_color`
(`.tools/t64/final-check.log`, `final-*.log`). The final continuous stair
underside then passed a further complete foyer trial (`.tools/t64/stair-final-run.log`);
`final-grand_foyer.log` contains that latest result. No C++ source changed; no Meson
verification is claimed for this Godot-only task. No previous intermittent
transition-close failure recurred in this run.
The initial stair trial carried a live Weston terminal upstairs, dropped it on
the gallery and typed in its real shell, but found a lower-tread descent snag.
After the collision correction, the complete ascent/descent/typing/tread-landing
trial passed. The final regression run additionally checks both stair guards.
Human physical-input, comfort and appearance-preference observation: **not observed**.

Each final fixed view sampled 120 process-frame intervals after settling:

| View | Median ms | p95 ms | Max ms |
| --- | ---: | ---: | ---: |
| entrance | 16.665 | 16.821 | 16.924 |
| spiral | 16.664 | 16.943 | 24.435 |
| upper-gallery | 16.659 | 16.88 | 24.031 |
| return | 16.69 | 29.012 | 30.794 |

These are bounded agent profiles, not a human latency/comfort gate or a claim
about all routes. Godot tracked static allocation stayed below 120 MB in these
samples; that excludes GPU texture memory and is not process RSS. Before
occlusion and local-light fading, the return view measured about 41.5ms median /
48.8ms p95. Retained diagnostics are `.tools/t64/profile.log` and `shadows.log`.
Changing the sun's shadow range did not help and is not part of the final change.

Reproduce with `tools/check-godot-study.sh` on a working display, or the private
Weston recipe in handoff 34 with `res://tests/grand_foyer.gd`, requiring
`GRAND_FOYER_OK levels=2 risers=40 failures=0` and no Godot errors. For only the
four fixed views/profiles, append `-- --foyer-captures-only`; this does not prove
stair interaction. The registered full trial allows 180 seconds. Local drivers
and exact commands are `.tools/t64/run.sh`, `godot-check.sh`, and `final-check.sh`.

Documentation links/task references, shell syntax and diffs were reviewed.
T63 and T64 complete this owner request. The unrelated T45 lifecycle gate remains
open, and no human gate is ticked. Nothing was pushed.
