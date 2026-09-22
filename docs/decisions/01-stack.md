# Decision: Technology Stack

**Context**:
The concept document suggested two plausible implementation stacks:
- C++ with wlroots
- Rust with Smithay

**Decision**: Start with C++ and wlroots.

**Reason**:
- C++/wlroots is the more common stack for Wayland compositors (used by sway, Wayfire, River, etc.)
- The IMPLEMENTATION-GUIDE.md was already written assuming the C++/wlroots stack
- wlroots provides partial compositors and helper utilities that can accelerate development
- More reference implementations and examples are available
- The Rust option remains available if we hit limitations

**Consequences**:
- Primary language: C++20
- Compositor foundation: wlroots
- Renderer: custom OpenGL/GLES2
- Build system: Meson
- Initial development: nested compositor using wlroots' libseat and xdg-desktop-portal-unity for session integration
n
## Notes

- wlroots is a library, not a compositor framework. We'll use its partial compositors as a starting point but will need to write most of the 3D world integration ourselves.
- We're not committed to C++ forever. The module boundaries in the architecture make a Rust rewrite feasible if desired later.
- The meson build system is common in wlroots ecosystem and works well with C++.
