#!/bin/sh
# Check for Elsewhere build and test dependencies.
status=0
for prog in meson ninja g++ gcc pkg-config wayland-scanner node; do
    if command -v "$prog" >/dev/null 2>&1; then echo "  $prog: ok"; else echo "  $prog: missing"; status=1; fi
done
for pkg in wayland-server wayland-client wayland-protocols wayland-egl egl glesv2 xkbcommon x11; do
    if pkg-config --exists "$pkg"; then echo "  $pkg: $(pkg-config --modversion "$pkg")"; else echo "  $pkg: missing"; status=1; fi
done
# Optional: only the human smoke test needs a real terminal client.
if command -v weston-terminal >/dev/null 2>&1; then echo "  weston-terminal: ok (optional)"; else echo "  weston-terminal: missing (optional; sudo pacman -S weston)"; fi
# Project 21 tools: download into tools/ per docs/TOOLCHAIN.md before use.
if [ -f "$PWD/tools/Godot_v4.7.2-stable_linux.x86_64" ]; then echo "  godot-4.7.2-stable: ok"; else echo "  godot-4.7.2-stable: missing (download per docs/TOOLCHAIN.md)"; fi
if [ -d "$PWD/tools/godot-cpp" ]; then echo "  godot-cpp-10.0.0-stable: ok"; else echo "  godot-cpp-10.0.0-stable: missing (git clone per docs/TOOLCHAIN.md)"; fi
if [ -f "$PWD/tools/blender-5.2.2-linux-x64/blender" ]; then echo "  blender-5.2.2-lts: ok"; else echo "  blender-5.2.2-lts: missing (download per docs/TOOLCHAIN.md)"; fi
# Project 21 world scaffold (P21-T01): basic validation.
if [ -f "$PWD/world/project.godot" ] && [ -f "$PWD/world/scenes/main.tscn" ] && [ -f "$PWD/world/scripts/game_world.gd" ]; then
    echo "  world scaffold (P21-T01): ok"
else
    echo "  world scaffold (P21-T01): missing"
    status=1
fi
[ "$status" -eq 0 ] && echo "All required dependencies are available." || echo "Some required dependencies are missing. See DEVELOPMENT.md."
exit $status
