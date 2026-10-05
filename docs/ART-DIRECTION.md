# Art direction: a study worth spending time in

This is the required visual direction for Project 21, not a claim about today's
prototype. Author and inspect real scenes. Do not answer a visual task with
another plan or make screenshots of a concept render stand in for runtime work.

## First room

Create a warm, believable, quietly luxurious study: oak/walnut furniture, a
textured timber floor, warm plaster walls, a lighter ceiling, soft daylight
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
