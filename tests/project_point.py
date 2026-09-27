#!/usr/bin/env python3
"""P2-T02: compute screen projection of a world point through the panel camera.

Usage: project_point.py CAM_X CAM_Y CAM_YAW CAM_PITCH WIN_W WIN_H POINT_X POINT_Y POINT_Z

Outputs: SCREEN_X SCREEN_Y

Uses the same math as the C++ compositor:
  - Column-major 4×4 matrices
  - Perspective with 90° FOV (PI/2), zNear=0.1, zFar=100
  - look_at(eye, target, up=(0,1,0))
  - Panel centred at origin, facing -Z, size 2×1.5
  - glViewport(0, 0, WIN_W, WIN_H)

This lets tests predict where a world point lands on the screenshot.
"""
import sys
import math


def mat4_identity():
    r = [0.0] * 16
    r[0] = r[5] = r[10] = r[15] = 1.0
    return r


def mat4_multiply(a, b):
    """Column-major multiply: r = a * b."""
    r = [0.0] * 16
    for col in range(4):
        for row in range(4):
            s = 0.0
            for i in range(4):
                s += a[i * 4 + row] * b[col * 4 + i]
            r[col * 4 + row] = s
    return r


def mat4_perspective(fov_rad, aspect, z_near, z_far):
    r = [0.0] * 16
    tan_half = math.tan(fov_rad * 0.5)
    r[0]  = 1.0 / (aspect * tan_half)
    r[5]  = 1.0 / tan_half
    r[10] = (z_far + z_near) / (z_near - z_far)
    r[11] = -1.0
    r[14] = (2.0 * z_far * z_near) / (z_near - z_far)
    return r


def mat4_look_at(eye_x, eye_y, eye_z, tx, ty, tz, up_x, up_y, up_z):
    """Column-major look_at matrix (matches C++ implementation)."""
    # forward
    fx = tx - eye_x
    fy = ty - eye_y
    fz = tz - eye_z
    fl = math.sqrt(fx * fx + fy * fy + fz * fz)
    if fl > 0:
        fx, fy, fz = fx / fl, fy / fl, fz / fl
    # up
    ux, uy, uz = up_x, up_y, up_z
    ul = math.sqrt(ux * ux + uy * uy + uz * uz)
    if ul > 0:
        ux, uy, uz = ux / ul, uy / ul, uz / ul
    # side
    sx = fy * uz - fz * uy
    sy = fz * ux - fx * uz
    sz = fx * uy - fy * ux
    sl = math.sqrt(sx * sx + sy * sy + sz * sz)
    if sl > 0:
        sx, sy, sz = sx / sl, sy / sl, sz / sl
    # recomputed up
    ux = sy * fz - sz * fy
    uy = sz * fx - sx * fz
    uz = sx * fy - sy * fx
    # dot(side, eye)
    d0 = -(sx * eye_x + sy * eye_y + sz * eye_z)
    # dot(up, eye)
    d1 = -(ux * eye_x + uy * eye_y + uz * eye_z)
    # dot(forward, eye)
    d2 = fx * eye_x + fy * eye_y + fz * eye_z
    r = [0.0] * 16
    # column 0: s
    r[0], r[4], r[8]  = sx, sy, sz
    # column 1: u
    r[1], r[5], r[9]  = ux, uy, uz
    # column 2: -f
    r[2], r[6], r[10] = -fx, -fy, -fz
    # column 3: translation (row 0=12, row 1=13, row 2=14)
    r[12], r[13], r[14] = d0, d1, d2
    r[15] = 1.0
    return r


def mat4_translate(x, y, z):
    r = mat4_identity()
    r[12] = x
    r[13] = y
    r[14] = z
    return r


def project_point(cam_x, cam_y, cam_z,
                  target_x, target_y, target_z,
                  win_w, win_h,
                  px, py, pz):
    """Return (screen_x, screen_y) for a world point."""
    fov_rad = math.pi / 2.0  # 90 degrees, matches C++ default
    aspect = win_w / win_h

    proj = mat4_perspective(fov_rad, aspect, 0.1, 100.0)
    view = mat4_look_at(cam_x, cam_y, cam_z,
                        target_x, target_y, target_z,
                        0, 1, 0)
    model = mat4_translate(0, 0, 0)  # panel at origin
    mvp = mat4_multiply(proj, mat4_multiply(view, model))

    # Transform point
    w = (mvp[3] * px + mvp[7] * py + mvp[11] * pz + mvp[15])
    if w == 0:
        return None
    ndc_x = (mvp[0] * px + mvp[4] * py + mvp[8] * pz + mvp[12]) / w
    ndc_y = (mvp[1] * px + mvp[5] * py + mvp[9] * pz + mvp[13]) / w

    # Viewport transform: NDC [-1,1] -> screen [0, win]
    screen_x = int((ndc_x + 1.0) * 0.5 * win_w)
    screen_y = int((ndc_y + 1.0) * 0.5 * win_h)

    return screen_x, screen_y


def main():
    if len(sys.argv) < 11:
        print(f"Usage: {sys.argv[0]} CAM_X CAM_Y CAM_Z "
              "TARGET_X TARGET_Y TARGET_Z WIN_W WIN_H "
              "POINT_X POINT_Y POINT_Z", file=sys.stderr)
        sys.exit(2)

    cam_x = float(sys.argv[1])
    cam_y = float(sys.argv[2])
    cam_z = float(sys.argv[3])
    target_x = float(sys.argv[4])
    target_y = float(sys.argv[5])
    target_z = float(sys.argv[6])
    win_w = int(sys.argv[7])
    win_h = int(sys.argv[8])
    px = float(sys.argv[9])
    py = float(sys.argv[10])
    pz = float(sys.argv[11])

    result = project_point(cam_x, cam_y, cam_z,
                           target_x, target_y, target_z,
                           win_w, win_h, px, py, pz)
    if result is None:
        print("POINT_BEHIND_CAMERA", file=sys.stdout)
        sys.exit(0)
    sx, sy = result
    print(f"{sx} {sy}")


if __name__ == "__main__":
    main()
