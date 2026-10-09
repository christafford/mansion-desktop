# Art direction: a study worth spending time in

This is the required visual direction for Project 21, not a claim about today's
prototype. Author and inspect real scenes. Do not answer a visual task with
another plan or make screenshots of a concept render stand in for runtime work.

## First room

Create a warm, believable, quietly luxurious study: oak/walnut furniture, a
textured timber floor, walnut-paneled walls (owner direction, 2026-10-06), a lighter ceiling, soft daylight
through a framed window and a warm desk lamp. Restrained brass/black details,
books, a rug and a plant give scale and life. Use a coherent palette and leave
space to walk. Avoid identical materials on all architectural surfaces,
overexposed HDRIs, oversized grain, noisy clutter and plastic-looking roughness.

The desk, chair, lamp, bookcase and monitor should read as actual objects with
thickness, plausible proportions and contact with the floor. Use pre-built
models; combine or model a missing monitor housing deliberately if the catalog
lacks one. Blockout boxes are allowed while arranging the room, not for passing
the finished-room gate. The fixed desk monitor keeps application content inside
its screen aperture.
The owner's explicit 2026-10-05 request additionally authorizes independent live
application panels (P21-T49): thin dark frames, restrained selection accents and
unlit client content, movable separately from furniture. These session-local
panels do not complete the finished-room or durable-workspace gates.

Choose Poly Haven models, texture sets and an HDRI by inspecting their current
catalog. Record chosen asset IDs and reasons; do not invent download URLs or
assume a particular desk/monitor exists. Match the art family and material
scale, rather than importing everything attractive. Add a connected secondary
room only after the first study and live-terminal interaction pass.

## Connected hallway (owner request, 2026-10-08)

A doorless opening opposite the desk leads into a 14m gallery: three closed
paneled doors on the left when leaving the study, two on the right, and one at
the end. Burgundy carpet with narrow bound borders, walnut wainscot/joinery,
warm plaster and brass oil lamps establish a quieter, dimmer space beyond the
study. Lamp reservoirs, glass chimneys, burners and flames must read as oil
fittings. The entrance and floor are continuous and walkable. T60 kept the doors
shut; T61 opens them into the exploration rooms described below.
This bounded extension does not claim broader multi-room persistence gates.

## Composition and light

- Arrival view frames the desk and its live monitor, with visible floor depth,
  architectural edges and a secondary focal point such as a shelf/window.
- Natural light establishes shape; practical lamps add warmth. Use supported
  environment, reflection and indirect-light techniques for the pinned renderer.
  Compare actual captures before choosing expensive effects. Prefer stable
  baked lighting for static geometry where appropriate; moving furniture needs
  a coherent dynamic/indirect lighting fallback.
- Inspect shadows, contact, exposure, texture tiling, normal orientation and
  reflection response. Tune materials individually; changing every surface to
  the same brown is not art direction.
- Monitor text is unlit, correctly oriented and high contrast. Its bezel can
  participate in lighting; the client pixels retain their colors. Check the
  readable application view as well as the attractive world view.
- Movement has comfortable speed, collision, safe spawn, optional reduced
  motion and quick return/teleport. No unavoidable camera bob or cinematic
  travel delay to reach a terminal.

## Required review loop

Render fixed views from arrival, desk close-up and the room's opposite corner,
plus the readable application view. Keep comparable before/after captures with
revision, camera, renderer, resolution and settings in the evidence record.
Inspect the images, write concrete defects, fix them, render again. Review
actual images using available vision tools; if none are available, collect the
captures and record visual review as blocked. Never claim to have seen them.
Don't stop at the first import that compiles. Continue until this rubric passes:

| Criterion | Pass evidence |
| --- | --- |
| Recognizable furniture | Desk/chair/monitor/lamp have correct shapes and scale |
| Surface variety | Floor, walls, ceiling and focal furniture are distinct PBR materials |
| Lighting | Readable forms, contact shadows, balanced exposure, no flat uniform wash |
| Placement | No floating props, major intersections, hidden screen or unusable walkway |
| Cohesion | Intentional palette, believable scale, limited clutter and focal hierarchy |
| Application | Real live pixels in the screen slot; readable full-size mode |
| Motion | Collision, focus and world return work without stuck input |
| Performance | Measured frame times and memory, with actual hardware recorded |

Set measurable performance goals at P21-T00: start with sustained 30 FPS at
1280x800 on the owner's Steam Deck and pursue 60 FPS where the quality/performance
tradeoff permits. Record percentile frame times and worst scene, not just average
FPS. A different machine's measurements are provisional. Preserve visual quality
while removing unnecessary draw calls, shadow costs and texture memory. There
is no deadline requiring premature acceptance, and no permission to optimize
away the room until it resembles the current bare polygons again.

Reminder creatures later should fit the study: one coherent character style,
clear silhouette, animation, proper floor contact and unobtrusive signaling.
They must represent a real reminder and provide an accessible list/dismissal
alternative. A static floating primitive does not pass the creature task.

## Exploration rooms (owner request, 2026-10-08; T61)

The six gallery doors now lead to named spaces for working and discovering,
rather than domestic bedrooms. Preserve the study and oil-lit gallery palette
as the connecting spine. Doors open inward on approach and stay open, with no
new input binding competing with application controls. Gold-lettered plaques identify
the destinations. Each space has a clear entry route and real collision-bearing
work surfaces, with color, silhouette and lighting as location cues:

- Long Library: green plaster, timber, stocked shelves and a reading runner.
- Atlas Room: cool plaster, large original expedition map, flat-file drawers
  and a small brass armillary.
- Winter Garden: framed glass roof admitting actual sky/sunlight, checkerboard
  stone, plants and a shallow fountain bowl.
- Cabinet Gallery: wine-colored walls, framed abstractions and two original
  sculptures on stone plinths, with a display/work table at the back.
- Inventor's Room: blue-gray plaster, mechanical drawing, toolboard, parts bins
  and a metal-topped workbench.
- Observatory: a wider final chamber with a star-chart ceiling, meridian inlays,
  central brass armillary and two worktables beneath constellation prints.

Reuse existing imported furniture and textured materials. Original authored
joinery and curved instrument/fountain meshes fill the specific catalog gaps;
small original SVG prints have recorded provenance in `world/art/README.md`.
Room geometry and session-local application placement are independent of live
process IDs. This extension does not implement restart restoration or complete
human comfort, general client compatibility, or persistent-workspace gates.

T62 extends rooms 01–04 from 6.4m to 12.8m deep, retaining their 3.8m width;
room 05 and the observatory retain their sizes. Repeat architectural trim and
lighting through the extension and spread landmarks toward the far work areas.
The library now uses eight tall, individually movable black bookcases with warm
plank interiors, layered crown mouldings, narrow iron rods and brass vessels.
A burgundy runner leads to the reading table's green leather inset and banker
lamp. See the [reference/provenance record](../world/art/README.md).
Common furniture throughout the six new rooms moves with the shared carry/push
controls. Books, lamps and sculptures stay with their supporting assemblies;
mounted art, toolboards, shelving, lights and the garden fountain stay fixed.
