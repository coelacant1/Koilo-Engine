// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ksl_types.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace ksl {

// --- Trigonometric ---

inline float sin(float x) { return std::sin(x); }
inline float cos(float x) { return std::cos(x); }
inline float tan(float x) { return std::tan(x); }
inline float asin(float x) { return std::asin(x); }
inline float acos(float x) { return std::acos(x); }
inline float atan(float x) { return std::atan(x); }
inline float atan(float y, float x) { return std::atan2(y, x); }

// --- Power / Exponential ---

inline float pow(float x, float y) { return std::pow(x, y); }
inline float sqrt(float x) { return std::sqrt(x); }
inline float inversesqrt(float x) {
    float xhalf = 0.5f * x;
    int32_t i;
    std::memcpy(&i, &x, sizeof(i));
    i = 0x5f3759df - (i >> 1);
    std::memcpy(&x, &i, sizeof(x));
    x *= (1.5f - xhalf * x * x);
    return x;
}
inline float exp(float x) { return std::exp(x); }
inline float log(float x) { return std::log(x); }
inline float exp2(float x) { return std::exp2(x); }
inline float log2(float x) { return std::log2(x); }

// --- Common ---

inline float abs(float x) { return std::fabs(x); }
inline float sign(float x) { return (x > 0.0f) ? 1.0f : (x < 0.0f) ? -1.0f : 0.0f; }
inline float floor(float x) { return std::floor(x); }
inline float ceil(float x) { return std::ceil(x); }
inline float round(float x) { return std::round(x); }
inline float fract(float x) { return x - std::floor(x); }
inline float mod(float x, float y) { return x - y * std::floor(x / y); }
inline float min(float a, float b) { return std::fmin(a, b); }
inline float max(float a, float b) { return std::fmax(a, b); }
inline float clamp(float x, float lo, float hi) { return std::fmin(std::fmax(x, lo), hi); }
inline int   clamp(int x, int lo, int hi) { return std::max(lo, std::min(x, hi)); }

// --- Interpolation ---

inline float mix(float a, float b, float t) { return a + (b - a) * t; }
inline float step(float edge, float x) { return x < edge ? 0.0f : 1.0f; }
inline float smoothstep(float e0, float e1, float x) {
    float t = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// --- Geometric (scalar helpers) ---

inline float length(const vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
inline float length(const vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
inline float length(const vec4& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w); }

inline float distance(const vec2& a, const vec2& b) { return length(a - b); }
inline float distance(const vec3& a, const vec3& b) { return length(a - b); }

inline float dot(const vec2& a, const vec2& b) { return a.x * b.x + a.y * b.y; }
inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float dot(const vec4& a, const vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline vec3 cross(const vec3& a, const vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline vec2 normalize(const vec2& v) { float l = length(v); return l > 0.0f ? v / l : vec2(0); }
inline vec3 normalize(const vec3& v) {
    float magSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (magSq <= 0.0f) return vec3(0);
    float inv = inversesqrt(magSq);
    return { v.x * inv, v.y * inv, v.z * inv };
}
inline vec4 normalize(const vec4& v) { float l = length(v); return l > 0.0f ? v / l : vec4(0); }

inline vec3 reflect(const vec3& I, const vec3& N) { return I - N * (2.0f * dot(N, I)); }
inline vec3 refract(const vec3& I, const vec3& N, float eta) {
    float d = dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - d * d);
    if (k < 0.0f) return vec3(0);
    return I * eta - N * (eta * d + std::sqrt(k));
}

// --- Component-wise vec2 ---

inline vec2 abs(const vec2& v) { return {abs(v.x), abs(v.y)}; }
inline vec2 floor(const vec2& v) { return {floor(v.x), floor(v.y)}; }
inline vec2 ceil(const vec2& v) { return {ceil(v.x), ceil(v.y)}; }
inline vec2 fract(const vec2& v) { return {fract(v.x), fract(v.y)}; }
inline vec2 mod(const vec2& v, float y) { return {mod(v.x, y), mod(v.y, y)}; }
inline vec2 min(const vec2& a, const vec2& b) { return {min(a.x, b.x), min(a.y, b.y)}; }
inline vec2 max(const vec2& a, const vec2& b) { return {max(a.x, b.x), max(a.y, b.y)}; }
inline vec2 clamp(const vec2& v, float lo, float hi) { return {clamp(v.x, lo, hi), clamp(v.y, lo, hi)}; }
inline vec2 mix(const vec2& a, const vec2& b, float t) { return {mix(a.x, b.x, t), mix(a.y, b.y, t)}; }
inline vec2 sin(const vec2& v) { return {sin(v.x), sin(v.y)}; }
inline vec2 cos(const vec2& v) { return {cos(v.x), cos(v.y)}; }

// --- Component-wise vec3 ---

inline vec3 abs(const vec3& v) { return {abs(v.x), abs(v.y), abs(v.z)}; }
inline vec3 floor(const vec3& v) { return {floor(v.x), floor(v.y), floor(v.z)}; }
inline vec3 ceil(const vec3& v) { return {ceil(v.x), ceil(v.y), ceil(v.z)}; }
inline vec3 fract(const vec3& v) { return {fract(v.x), fract(v.y), fract(v.z)}; }
inline vec3 mod(const vec3& v, float y) { return {mod(v.x, y), mod(v.y, y), mod(v.z, y)}; }
inline vec3 min(const vec3& a, const vec3& b) { return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)}; }
inline vec3 max(const vec3& a, const vec3& b) { return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)}; }
inline vec3 clamp(const vec3& v, float lo, float hi) { return {clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi)}; }
inline vec3 clamp(const vec3& v, const vec3& lo, const vec3& hi) { return {clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)}; }
inline vec3 mix(const vec3& a, const vec3& b, float t) { return {mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t)}; }
inline vec3 sin(const vec3& v) { return {sin(v.x), sin(v.y), sin(v.z)}; }
inline vec3 cos(const vec3& v) { return {cos(v.x), cos(v.y), cos(v.z)}; }
inline vec3 pow(const vec3& v, const vec3& e) { return {pow(v.x, e.x), pow(v.y, e.y), pow(v.z, e.z)}; }
inline vec3 sqrt(const vec3& v) { return {sqrt(v.x), sqrt(v.y), sqrt(v.z)}; }
inline vec3 step(const vec3& edge, const vec3& x) { return {step(edge.x, x.x), step(edge.y, x.y), step(edge.z, x.z)}; }
inline vec3 step(float edge, const vec3& x) { return {step(edge, x.x), step(edge, x.y), step(edge, x.z)}; }

// --- Component-wise vec4 ---

inline vec4 abs(const vec4& v) { return {abs(v.x), abs(v.y), abs(v.z), abs(v.w)}; }
inline vec4 floor(const vec4& v) { return {floor(v.x), floor(v.y), floor(v.z), floor(v.w)}; }
inline vec4 ceil(const vec4& v) { return {ceil(v.x), ceil(v.y), ceil(v.z), ceil(v.w)}; }
inline vec4 fract(const vec4& v) { return {fract(v.x), fract(v.y), fract(v.z), fract(v.w)}; }
inline vec4 clamp(const vec4& v, float lo, float hi) { return {clamp(v.x, lo, hi), clamp(v.y, lo, hi), clamp(v.z, lo, hi), clamp(v.w, lo, hi)}; }
inline vec4 min(const vec4& a, const vec4& b) { return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)}; }
inline vec4 max(const vec4& a, const vec4& b) { return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)}; }
inline vec4 mix(const vec4& a, const vec4& b, float t) { return {mix(a.x, b.x, t), mix(a.y, b.y, t), mix(a.z, b.z, t), mix(a.w, b.w, t)}; }
inline vec4 step(const vec4& edge, const vec4& x) { return {step(edge.x, x.x), step(edge.y, x.y), step(edge.z, x.z), step(edge.w, x.w)}; }
inline vec4 step(float edge, const vec4& x) { return {step(edge, x.x), step(edge, x.y), step(edge, x.z), step(edge, x.w)}; }

// --- Extended: Linear remap ---

inline float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (out_max - out_min) * (x - in_min) / (in_max - in_min);
}

// --- Extended: Cosine interpolation ---

inline float cosineInterpolation(float a, float b, float t) {
    float t2 = (1.0f - cos(t * 3.14159265f)) / 2.0f;
    return a * (1.0f - t2) + b * t2;
}

// --- Extended: 2D rotation about a center point ---

inline vec2 rotate2d(const vec2& p, float angleDeg, const vec2& center) {
    float rad = angleDeg * 3.14159265f / 180.0f;
    float c = cos(rad);
    float s = sin(rad);
    vec2 d = p - center;
    return vec2(d.x * c - d.y * s + center.x, d.x * s + d.y * c + center.y);
}

// --- Extended: Hue shift in RGB space (Rodrigues rotation around (1,1,1)/sqrt(3)) ---

inline vec3 hueShift(const vec3& c, float angleDeg) {
    float rad = angleDeg * 3.14159265f / 180.0f;
    float cosA = cos(rad);
    float sinA = sin(rad);
    float k = (1.0f - cosA) / 3.0f;
    float sq = sinA * 0.57735026919f;
    return vec3(
        c.x * (cosA + k) + c.y * (k - sq) + c.z * (k + sq),
        c.x * (k + sq) + c.y * (cosA + k) + c.z * (k - sq),
        c.x * (k - sq) + c.y * (k + sq) + c.z * (cosA + k)
    );
}

// --- Extended: Hash and noise for procedural effects ---

inline float hash(float n) {
    return fract(sin(n) * 43758.5453123f);
}

inline float noise3d(const vec3& p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (vec3(3.0f) - f * 2.0f);
    float n = i.x + i.y * 157.0f + i.z * 113.0f;
    return mix(mix(mix(hash(n + 0.0f),   hash(n + 1.0f),   u.x),
                   mix(hash(n + 157.0f), hash(n + 158.0f), u.x), u.y),
               mix(mix(hash(n + 113.0f), hash(n + 114.0f), u.x),
                   mix(hash(n + 270.0f), hash(n + 271.0f), u.x), u.y), u.z) * 2.0f - 1.0f;
}

} // namespace ksl
