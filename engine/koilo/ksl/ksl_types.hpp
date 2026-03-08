// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cmath>
#include "../registry/reflect_macros.hpp"

namespace ksl {

struct vec2;
struct vec3;
struct vec4;

struct vec2 {
    float x, y;

    vec2() : x(0), y(0) {}
    explicit vec2(float s) : x(s), y(s) {}
    vec2(float x, float y) : x(x), y(y) {}

    vec2 operator+(const vec2& b) const { return {x + b.x, y + b.y}; }
    vec2 operator-(const vec2& b) const { return {x - b.x, y - b.y}; }
    vec2 operator*(const vec2& b) const { return {x * b.x, y * b.y}; }
    vec2 operator/(const vec2& b) const { return {x / b.x, y / b.y}; }
    vec2 operator*(float s) const { return {x * s, y * s}; }
    vec2 operator/(float s) const { return {x / s, y / s}; }
    vec2 operator-() const { return {-x, -y}; }

    vec2& operator+=(const vec2& b) { x += b.x; y += b.y; return *this; }
    vec2& operator-=(const vec2& b) { x -= b.x; y -= b.y; return *this; }
    vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    bool operator==(const vec2& b) const { return x == b.x && y == b.y; }
    bool operator!=(const vec2& b) const { return !(*this == b); }

    float& operator[](int i) { return (&x)[i]; }
    float  operator[](int i) const { return (&x)[i]; }

    KL_BEGIN_FIELDS(vec2)
        KL_FIELD(vec2, x, "X", 0, 0),
        KL_FIELD(vec2, y, "Y", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(vec2)
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(vec2)
        KL_CTOR0(vec2),
        KL_CTOR(vec2, float),
        KL_CTOR(vec2, float, float)
    KL_END_DESCRIBE(vec2)

};

inline vec2 operator*(float s, const vec2& v) { return {s * v.x, s * v.y}; }

struct vec3 {
    float x, y, z;

    vec3() : x(0), y(0), z(0) {}
    explicit vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec2& v, float z) : x(v.x), y(v.y), z(z) {}

    vec3 operator+(const vec3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    vec3 operator-(const vec3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    vec3 operator*(const vec3& b) const { return {x * b.x, y * b.y, z * b.z}; }
    vec3 operator/(const vec3& b) const { return {x / b.x, y / b.y, z / b.z}; }
    vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    vec3 operator-() const { return {-x, -y, -z}; }

    vec3& operator+=(const vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    vec3& operator-=(const vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
    vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

    bool operator==(const vec3& b) const { return x == b.x && y == b.y && z == b.z; }
    bool operator!=(const vec3& b) const { return !(*this == b); }

    float& operator[](int i) { return (&x)[i]; }
    float  operator[](int i) const { return (&x)[i]; }

    // Swizzle methods (C++ can't do GLSL-style .xy property access)
    vec2 xy() const { return {x, y}; }
    vec2 xz() const { return {x, z}; }
    vec2 yz() const { return {y, z}; }

    KL_BEGIN_FIELDS(vec3)
        KL_FIELD(vec3, x, "X", 0, 0),
        KL_FIELD(vec3, y, "Y", 0, 0),
        KL_FIELD(vec3, z, "Z", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(vec3)
        KL_METHOD_AUTO(vec3, xy, "Xy"),
        KL_METHOD_AUTO(vec3, xz, "Xz"),
        KL_METHOD_AUTO(vec3, yz, "Yz")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(vec3)
        KL_CTOR0(vec3),
        KL_CTOR(vec3, float),
        KL_CTOR(vec3, float, float, float)
    KL_END_DESCRIBE(vec3)

};

inline vec3 operator*(float s, const vec3& v) { return {s * v.x, s * v.y, s * v.z}; }

struct vec4 {
    float x, y, z, w;

    vec4() : x(0), y(0), z(0), w(0) {}
    explicit vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    vec4(const vec2& v, float z, float w) : x(v.x), y(v.y), z(z), w(w) {}

    vec4 operator+(const vec4& b) const { return {x + b.x, y + b.y, z + b.z, w + b.w}; }
    vec4 operator-(const vec4& b) const { return {x - b.x, y - b.y, z - b.z, w - b.w}; }
    vec4 operator*(const vec4& b) const { return {x * b.x, y * b.y, z * b.z, w * b.w}; }
    vec4 operator/(const vec4& b) const { return {x / b.x, y / b.y, z / b.z, w / b.w}; }
    vec4 operator*(float s) const { return {x * s, y * s, z * s, w * s}; }
    vec4 operator/(float s) const { return {x / s, y / s, z / s, w / s}; }
    vec4 operator-() const { return {-x, -y, -z, -w}; }

    vec4& operator+=(const vec4& b) { x += b.x; y += b.y; z += b.z; w += b.w; return *this; }
    vec4& operator-=(const vec4& b) { x -= b.x; y -= b.y; z -= b.z; w -= b.w; return *this; }
    vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    vec4& operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }

    bool operator==(const vec4& b) const { return x == b.x && y == b.y && z == b.z && w == b.w; }
    bool operator!=(const vec4& b) const { return !(*this == b); }

    float& operator[](int i) { return (&x)[i]; }
    float  operator[](int i) const { return (&x)[i]; }

    // Swizzle methods
    vec2 xy() const { return {x, y}; }
    vec2 xz() const { return {x, z}; }
    vec2 yz() const { return {y, z}; }
    vec3 xyz() const { return {x, y, z}; }
    vec3 rgb() const { return {x, y, z}; }

    KL_BEGIN_FIELDS(vec4)
        KL_FIELD(vec4, x, "X", 0, 0),
        KL_FIELD(vec4, y, "Y", 0, 0),
        KL_FIELD(vec4, z, "Z", 0, 0),
        KL_FIELD(vec4, w, "W", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(vec4)
        KL_METHOD_AUTO(vec4, xy, "Xy"),
        KL_METHOD_AUTO(vec4, xz, "Xz"),
        KL_METHOD_AUTO(vec4, yz, "Yz"),
        KL_METHOD_AUTO(vec4, xyz, "Xyz"),
        KL_METHOD_AUTO(vec4, rgb, "Rgb")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(vec4)
        KL_CTOR0(vec4),
        KL_CTOR(vec4, float),
        KL_CTOR(vec4, float, float, float, float)
    KL_END_DESCRIBE(vec4)

};

inline vec4 operator*(float s, const vec4& v) { return {s * v.x, s * v.y, s * v.z, s * v.w}; }

} // namespace ksl
