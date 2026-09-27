#pragma once

// Header-only 3D math for Project 2+ (P2-T01).
// No dependencies; C++20 compatible.

#include <cmath>
#include <cstdint>

namespace math {

inline constexpr double PI = 3.14159265358979323846;

// ── vec3 ──────────────────────────────────────────────────────────────────

struct vec3 {
    float x = 0, y = 0, z = 0;

    constexpr vec3() = default;
    constexpr vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    constexpr vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    constexpr float operator[](int i) const { return (&x)[i]; }
    float& operator[](int i) { return (&x)[i]; }
};

inline constexpr vec3 operator*(float s, const vec3& v) { return v * s; }
inline vec3 operator-(const vec3& v) { return {-v.x, -v.y, -v.z}; }

inline float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline vec3 cross(vec3 a, vec3 b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

inline float length(vec3 v) { return std::sqrt(dot(v, v)); }

inline vec3 normalize(vec3 v) {
    float l = length(v);
    return l > 0 ? v * (1.0f / l) : vec3{0, 0, 0};
}

// ── mat4 (column-major, OpenGL convention) ────────────────────────────────

struct mat4 {
    float m[16]{}; // column-major: m[col * 4 + row]

    constexpr mat4() = default;

    static mat4 identity() {
        mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    static mat4 multiply(const mat4& a, const mat4& b) {
        mat4 r{};
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                for (int i = 0; i < 4; ++i)
                    r.m[col * 4 + row] += a.m[i * 4 + row] * b.m[col * 4 + i];
        return r;
    }

    vec3 transform_point(const vec3& p) const {
        float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        return {
            (m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12]) / w,
            (m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13]) / w,
            (m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]) / w
        };
    }

    vec3 transform_dir(const vec3& d) const {
        return {
            m[0] * d.x + m[4] * d.y + m[8] * d.z,
            m[1] * d.x + m[5] * d.y + m[9] * d.z,
            m[2] * d.x + m[6] * d.y + m[10] * d.z
        };
    }
};

// ── construction helpers ──────────────────────────────────────────────────

inline mat4 perspective(float fov_rad, float aspect, float z_near, float z_far) {
    mat4 r{};
    float tan_half_fov = std::tan(fov_rad * 0.5f);
    r.m[0]  = 1.0f / (aspect * tan_half_fov);
    r.m[5]  = 1.0f / tan_half_fov;
    r.m[10] = (z_far + z_near) / (z_near - z_far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * z_far * z_near) / (z_near - z_far);
    return r;
}

inline mat4 look_at(const vec3& eye, const vec3& target, const vec3& up) {
    vec3 f = normalize(target - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 r{};
    r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
    r.m[2] =-f.x; r.m[6] =-f.y; r.m[10] =-f.z;
    r.m[12] =-dot(s, eye);
    r.m[13] =-dot(u, eye);
    r.m[14] = dot(f, eye);
    r.m[15] = 1.0f;
    return r;
}

inline mat4 translate(float x, float y, float z) {
    mat4 r = mat4::identity();
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
}

inline mat4 rotate_y(float angle_rad) {
    mat4 r = mat4::identity();
    float c = std::cos(angle_rad);
    float sn = std::sin(angle_rad);
    r.m[0] = c;  r.m[2] = sn;  // column 0: (cos, 0, -sin, 0)  -- indices [0], [4]=0, [8]
    r.m[8] =-sn; r.m[10] = c;  // column 2: (sin, 0,  cos, 0)  -- indices [2], [6]=0, [10]
    return r;
}

inline mat4 rotate_x(float angle_rad) {
    mat4 r = mat4::identity();
    float c = std::cos(angle_rad);
    float sn = std::sin(angle_rad);
    r.m[5] = c;  r.m[6] =-sn;  // column 1: (0, cos,  sin, 0)  -- indices [5], [9]
    r.m[9] = sn; r.m[10] = c;  // column 2: (0, -sin, cos, 0)  -- indices [6], [10]
    return r;
}

} // namespace math
