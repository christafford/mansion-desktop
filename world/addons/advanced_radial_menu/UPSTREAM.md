# Advanced Radial Menu

Source: https://github.com/diklor/advanced_radial_menu
Revision: `decd8679efd19947a54bbe99fc5644846322f745` (downloaded 2026-10-04).
MIT license retained in LICENSE.txt. Runtime script and icon only; editor plugin
and examples are unnecessary for the launcher.

Local patch: `line_rotation_offset_default` is float instead of int, allowing
-22.5 degrees for eight slots with the first slot centered at the top.
All other upstream runtime behavior is preserved.
The file also gains a final newline; upstream blank-line indentation is retained.

SHA-256:

| File | Upstream | Vendored |
| --- | --- | --- |
| radial_menu_class.gd | `a2b2c357b3345452d57f81c48096b1cb4adef679fc017b2ce887fcf778a584eb` | `4256e0e1b1b4afe5f9fadff0a30fa89460420c04c965042ded7e063c349d3620` |
| icon.svg | `982e460f989d6c65fcaa3381aad87d2d99c513f6c578d27c979443e206838b43` | unchanged |
| LICENSE.txt | `f29614306c579e4dfe897086e852f43cf633805dfa42a4e749bb83ad9181f22f` | unchanged |
