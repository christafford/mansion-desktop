// P2-T01: unit tests for src/math.h
// Run: meson test -C build math

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../src/math3d.h"

static int failures = 0;

static void check(bool cond, const char* file, int line, const char* desc) {
    if (!cond) {
        fprintf(stderr, "FAIL %s:%d: %s\n", file, line, desc);
        ++failures;
    }
}

static void check_near(float a, float b, float eps, const char* desc) {
    check(std::fabs(a - b) < eps, __FILE__, __LINE__, desc);
}

static void check_near_vec3(const math::vec3& a, const math::vec3& b, float eps, const char* desc) {
    check_near(a.x, b.x, eps, desc);
    check_near(a.y, b.y, eps, desc);
    check_near(a.z, b.z, eps, desc);
}

// ── vec3 ─────────────────────────────────────────────────────────────────

static void test_vec3() {
    math::vec3 a{1, 2, 3};
    math::vec3 b{4, 5, 6};

    auto sum = a + b;
    check_near_vec3(sum, {5, 7, 9}, 1e-6, "vec3 addition");

    auto diff = b - a;
    check_near_vec3(diff, {3, 3, 3}, 1e-6, "vec3 subtraction");

    auto scaled = a * 2.0f;
    check_near_vec3(scaled, {2, 4, 6}, 1e-6, "vec3 scalar mul");

    check_near(math::dot(a, b), 32.0f, 1e-6, "dot product");

    auto cr = math::cross(a, b);
    check_near_vec3(cr, {-3, 6, -3}, 1e-6, "cross product");

    math::vec3 n = math::normalize(math::vec3{0, 3, 0});
    check_near_vec3(n, {0, 1, 0}, 1e-4, "normalize +Y");

    check_near(math::length(math::vec3{3, 4, 0}), 5.0f, 1e-4, "length 3-4-5");
}

// ── mat4 ─────────────────────────────────────────────────────────────────

static void test_identity() {
    auto id = math::mat4::identity();
    for (int i = 0; i < 4; ++i)
        check_near(id.m[i * 4 + i], 1.0f, 1e-6, "identity diagonal");
    for (int i = 0; i < 16; ++i)
        if (i % 5 == 0) continue; else
        check_near(id.m[i], 0.0f, 1e-6, "identity zero");
}

static void test_multiply() {
    auto id = math::mat4::identity();
    auto v  = math::translate(1, 2, 3);
    auto r1 = math::mat4::multiply(id, v);
    auto r2 = math::mat4::multiply(v, id);
    // Identity should be neutral
    for (int i = 0; i < 16; ++i)
        check_near(r1.m[i], v.m[i], 1e-5, "multiply left identity");
    for (int i = 0; i < 16; ++i)
        check_near(r2.m[i], v.m[i], 1e-5, "multiply right identity");
}

static void test_transform_point() {
    auto t = math::translate(5, 10, 15);
    auto p = t.transform_point({1, 1, 1});
    check_near_vec3(p, {6, 11, 16}, 1e-4, "translate transform_point");

    auto id = math::mat4::identity();
    auto ip = id.transform_point({3, 4, 5});
    check_near_vec3(ip, {3, 4, 5}, 1e-4, "identity transform_point");
}

// ── perspective ──────────────────────────────────────────────────────────

static void test_perspective() {
    // 90-degree FOV, aspect 1: near plane = ±1 at z = -near
    auto p = math::perspective(math::PI * 0.5f, 1.0f, 0.1f, 100.0f);
    check_near(p.m[0], 1.0f, 1e-4, "perspective diagonal[0]");
    check_near(p.m[5], 1.0f, 1e-4, "perspective diagonal[5]");
}

// ── look_at ──────────────────────────────────────────────────────────────

static void test_look_at() {
    // Camera at +Z looking toward origin, up is +Y
    auto eye    = math::vec3{0, 0, 5};
    auto target = math::vec3{0, 0, 0};
    auto up     = math::vec3{0, 1, 0};
    auto view   = math::look_at(eye, target, up);

    // The forward vector of the view matrix's 3rd row (negated) should point to target
    auto fwd = math::normalize(math::vec3{-view.m[8], -view.m[9], -view.m[10]});
    check_near_vec3(fwd, {0, 0, -1}, 1e-4, "look_at forward");
}

// ── rotate ───────────────────────────────────────────────────────────────

static void test_rotate_y() {
    auto ry90 = math::rotate_y(math::PI * 0.5f);

    // (1,0,0) rotated 90 deg around Y should go to (0,0,1)
    auto p = ry90.transform_point({1, 0, 0});
    check_near(p.x, 0.0f, 1e-3, "rotate_y 90 x→0");
    check_near(p.z, 1.0f, 1e-3, "rotate_y 90 x→z");
}

static void test_rotate_x() {
    auto rx90 = math::rotate_x(math::PI * 0.5f);

    // (0,0,1) rotated 90 deg around X should go to (0,1,0)
    auto p = rx90.transform_point({0, 0, 1});
    check_near(p.y, 1.0f, 1e-3, "rotate_x 90 z→y");
    check_near(p.z, 0.0f, 1e-3, "rotate_x 90 z→0");
}

// ── full pipeline: perspective × look_at × translate ─────────────────────

static void test_pipeline() {
    // Project a known point: camera at (0,0,10) looking at origin,
    // point at world origin. After look_at the point is (0,0,-10).
    // Perspective should place it near the centre of the near plane.
    auto proj = math::perspective(math::PI * 0.5f, 1.0f, 0.1f, 100.0f);
    auto view = math::look_at({0, 0, 10}, {0, 0, 0}, {0, 1, 0});
    auto vp   = math::mat4::multiply(proj, view);

    auto p = vp.transform_point({0, 0, 0}); // origin
    check_near(p.x, 0.0f, 1e-3, "pipeline: centre X");
    check_near(p.y, 0.0f, 1e-3, "pipeline: centre Y");
}

// ── main ─────────────────────────────────────────────────────────────────

int main() {
    test_vec3();
    test_identity();
    test_multiply();
    test_transform_point();
    test_perspective();
    test_look_at();
    test_rotate_y();
    test_rotate_x();
    test_pipeline();

    if (failures) {
        fprintf(stderr, "%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All %d test groups passed\n", 9);
    return 0;
}
