#!/usr/bin/env python3
"""Check the color of a pixel in a PPM (P6) file.

Usage: ppm_pixel.py FILE X Y RRGGBB [tolerance]

Checks that the pixel at (X, Y) is within `tolerance` of the given
hex color (defaults to 15).  Prints PASS or FAIL on stdout and
exits 0/1 accordingly.
"""
import sys

def main():
    if len(sys.argv) < 5:
        print(f"Usage: {sys.argv[0]} FILE X Y RRGGBB [tolerance]", file=sys.stderr)
        sys.exit(2)

    filepath = sys.argv[1]
    x = int(sys.argv[2])
    y = int(sys.argv[3])
    expected_hex = sys.argv[4]
    tolerance = int(sys.argv[5]) if len(sys.argv) > 5 else 15

    if len(expected_hex) != 6:
        print(f"Error: color must be 6 hex digits, got '{expected_hex}'", file=sys.stderr)
        sys.exit(2)

    expected_r = int(expected_hex[0:2], 16)
    expected_g = int(expected_hex[2:4], 16)
    expected_b = int(expected_hex[4:6], 16)

    with open(filepath, "rb") as f:
        header = f.readline().decode("ascii").strip()
        assert header == "P6", f"Expected P6 PPM, got '{header}'"

        # Skip comments
        line = f.readline().decode("ascii").strip()
        while line.startswith("#"):
            line = f.readline().decode("ascii").strip()

        parts = line.split()
        w, h = int(parts[0]), int(parts[1])
        maxval = int(parts[2]) if len(parts) > 2 else 255

        assert 0 <= x < w, f"X={x} out of range [0, {w})"
        assert 0 <= y < h, f"Y={y} out of range [0, {h})"

        # Skip the maxval line if present on its own line
        if len(parts) <= 2:
            f.readline()
        pixel_data = f.read()

        offset = (y * w + x) * 3
        actual_r = pixel_data[offset]
        actual_g = pixel_data[offset + 1]
        actual_b = pixel_data[offset + 2]

    dr = abs(actual_r - expected_r)
    dg = abs(actual_g - expected_g)
    db = abs(actual_b - expected_b)

    if dr <= tolerance and dg <= tolerance and db <= tolerance:
        print(f"PASS pixel ({x},{y})=({actual_r},{actual_g},{actual_b}) ~= ({expected_r},{expected_g},{expected_b}) tol={tolerance}")
        sys.exit(0)
    else:
        print(f"FAIL pixel ({x},{y})=({actual_r},{actual_g},{actual_b}) ~= ({expected_r},{expected_g},{expected_b}) tol={tolerance}")
        print(f"  deltas: R={dr} G={dg} B={db}")
        sys.exit(1)

if __name__ == "__main__":
    main()
