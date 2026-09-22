#!/usr/bin/sh
# Check for Mansion Desktop build dependencies

echo "Checking dependencies..."

required Programs="meson"
missing=""

for prog in $required Programs; do
    if ! command -v $prog >/dev/null; then
        echo "  $prog: missing"
        missing="1"
    else
        echo "  $prog: ok"
    fi
done

required_pkg="wayland-server xkbcommon egl wayland-egl"
missing_pkg=""

for pkg in $required_pkg; do
    if ! pkg-config --exists $pkg; then
        echo "  $pkg: missing"
        missing_pkg="1"
    else
        echo "  $pkg: ok"
    fi
done

if [ -z "$missing" ] && [ -z "$missing_pkg" ]; then
    echo "All dependencies are available."
    exit 0
else
    echo "Some dependencies are missing."
    echo "See DEVELOPMENT.md for setup instructions."
    exit 1
fi
