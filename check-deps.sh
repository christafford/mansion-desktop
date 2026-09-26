#!/bin/sh
# Check for Mansion Desktop build and test dependencies.
status=0
for prog in meson ninja g++ gcc pkg-config wayland-scanner node; do
    if command -v "$prog" >/dev/null 2>&1; then echo "  $prog: ok"; else echo "  $prog: missing"; status=1; fi
done
for pkg in wayland-server wayland-client wayland-protocols wayland-egl egl glesv2 xkbcommon x11; do
    if pkg-config --exists "$pkg"; then echo "  $pkg: $(pkg-config --modversion "$pkg")"; else echo "  $pkg: missing"; status=1; fi
done
# Optional: only the human smoke test needs a real terminal client.
if command -v weston-terminal >/dev/null 2>&1; then echo "  weston-terminal: ok (optional)"; else echo "  weston-terminal: missing (optional; sudo pacman -S weston)"; fi
[ "$status" -eq 0 ] && echo "All required dependencies are available." || echo "Some required dependencies are missing. See DEVELOPMENT.md."
exit $status
