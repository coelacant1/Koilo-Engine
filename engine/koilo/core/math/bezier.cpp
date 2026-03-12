// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/bezier.hpp>
#include <cmath>
#include <algorithm>

namespace koilo {

namespace {

void QuadFlattenRecursive(std::vector<Vector2D>& out,
                          const Vector2D& a, const Vector2D& b, const Vector2D& c,
                          float tolerance, int depth) {
    if (depth > 16) { out.push_back(a); return; }

    // Flatness test: distance from control point to midpoint of line a->c
    Vector2D mid = (a + c) * 0.5f;
    float dx = b.X - mid.X;
    float dy = b.Y - mid.Y;
    if (dx * dx + dy * dy <= tolerance * tolerance) {
        out.push_back(a);
        return;
    }

    // Subdivide at t=0.5
    Vector2D m0 = (a + b) * 0.5f;
    Vector2D m1 = (b + c) * 0.5f;
    Vector2D mp = (m0 + m1) * 0.5f;
    QuadFlattenRecursive(out, a, m0, mp, tolerance, depth + 1);
    QuadFlattenRecursive(out, mp, m1, c, tolerance, depth + 1);
}

int SolveQuadratic(float a, float b, float c, float roots[2]) {
    if (std::fabs(a) < 1e-6f) {
        if (std::fabs(b) < 1e-6f) return 0;
        roots[0] = -c / b;
        return 1;
    }
    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) return 0;
    float sqrtDisc = std::sqrt(disc);
    float inv2a = 0.5f / a;
    roots[0] = (-b + sqrtDisc) * inv2a;
    roots[1] = (-b - sqrtDisc) * inv2a;
    return 2;
}

void CubicFlattenRecursive(std::vector<Vector2D>& out,
                            const CubicBezier& curve,
                            float tolerance, int depth) {
    if (depth > 16) { out.push_back(curve.p0); return; }

    // Flatness: max distance of control points from chord
    Vector2D d1 = curve.p1 - Vector2D::LERP(curve.p0, curve.p3, 1.0f / 3.0f);
    Vector2D d2 = curve.p2 - Vector2D::LERP(curve.p0, curve.p3, 2.0f / 3.0f);
    float maxDist = std::max(d1.X * d1.X + d1.Y * d1.Y,
                             d2.X * d2.X + d2.Y * d2.Y);
    if (maxDist <= tolerance * tolerance) {
        out.push_back(curve.p0);
        return;
    }

    CubicBezier left, right;
    curve.Split(0.5f, left, right);
    CubicFlattenRecursive(out, left, tolerance, depth + 1);
    CubicFlattenRecursive(out, right, tolerance, depth + 1);
}

} // anonymous namespace

// --- QuadBezier ------------------------------------------------------

Vector2D QuadBezier::Evaluate(float t) const {
    float u = 1.0f - t;
    return p0 * (u * u) + p1 * (2.0f * u * t) + p2 * (t * t);
}

Vector2D QuadBezier::Derivative(float t) const {
    float u = 1.0f - t;
    return (p1 - p0) * (2.0f * u) + (p2 - p1) * (2.0f * t);
}

void QuadBezier::Split(float t, QuadBezier& left, QuadBezier& right) const {
    Vector2D m0 = Vector2D::LERP(p0, p1, t);
    Vector2D m1 = Vector2D::LERP(p1, p2, t);
    Vector2D mid = Vector2D::LERP(m0, m1, t);
    left  = { p0, m0, mid };
    right = { mid, m1, p2 };
}

void QuadBezier::BoundingBox(Vector2D& outMin, Vector2D& outMax) const {
    outMin = Vector2D::Minimum(p0, p2);
    outMax = Vector2D::Maximum(p0, p2);
    // Check extrema: derivative = 0 -> t = (p0 - p1) / (p0 - 2*p1 + p2)
    for (int axis = 0; axis < 2; ++axis) {
        float a = (axis == 0) ? p0.X : p0.Y;
        float b = (axis == 0) ? p1.X : p1.Y;
        float c = (axis == 0) ? p2.X : p2.Y;
        float denom = a - 2.0f * b + c;
        if (std::fabs(denom) > 1e-6f) {
            float t = (a - b) / denom;
            if (t > 0.0f && t < 1.0f) {
                Vector2D pt = Evaluate(t);
                outMin = Vector2D::Minimum(outMin, pt);
                outMax = Vector2D::Maximum(outMax, pt);
            }
        }
    }
}

void QuadBezier::Flatten(std::vector<Vector2D>& out, float tolerance) const {
    QuadFlattenRecursive(out, p0, p1, p2, tolerance, 0);
    out.push_back(p2);
}

// --- CubicBezier -----------------------------------------------------

Vector2D CubicBezier::Evaluate(float t) const {
    float u = 1.0f - t;
    float u2 = u * u;
    float t2 = t * t;
    return p0 * (u2 * u) + p1 * (3.0f * u2 * t) +
           p2 * (3.0f * u * t2) + p3 * (t2 * t);
}

Vector2D CubicBezier::Derivative(float t) const {
    float u = 1.0f - t;
    return (p1 - p0) * (3.0f * u * u) +
           (p2 - p1) * (6.0f * u * t) +
           (p3 - p2) * (3.0f * t * t);
}

void CubicBezier::Split(float t, CubicBezier& left, CubicBezier& right) const {
    Vector2D m01 = Vector2D::LERP(p0, p1, t);
    Vector2D m12 = Vector2D::LERP(p1, p2, t);
    Vector2D m23 = Vector2D::LERP(p2, p3, t);
    Vector2D m012 = Vector2D::LERP(m01, m12, t);
    Vector2D m123 = Vector2D::LERP(m12, m23, t);
    Vector2D mid  = Vector2D::LERP(m012, m123, t);
    left  = { p0, m01, m012, mid };
    right = { mid, m123, m23, p3 };
}

void CubicBezier::BoundingBox(Vector2D& outMin, Vector2D& outMax) const {
    outMin = Vector2D::Minimum(p0, p3);
    outMax = Vector2D::Maximum(p0, p3);
    // Solve derivative = 0 per axis (quadratic in t)
    for (int axis = 0; axis < 2; ++axis) {
        float v0 = (axis == 0) ? p0.X : p0.Y;
        float v1 = (axis == 0) ? p1.X : p1.Y;
        float v2 = (axis == 0) ? p2.X : p2.Y;
        float v3 = (axis == 0) ? p3.X : p3.Y;
        float a = -v0 + 3.0f * v1 - 3.0f * v2 + v3;
        float b =  2.0f * v0 - 4.0f * v1 + 2.0f * v2;
        float c = -v0 + v1;
        float roots[2];
        int n = SolveQuadratic(a, b, c, roots);
        for (int i = 0; i < n; ++i) {
            if (roots[i] > 0.0f && roots[i] < 1.0f) {
                Vector2D pt = Evaluate(roots[i]);
                outMin = Vector2D::Minimum(outMin, pt);
                outMax = Vector2D::Maximum(outMax, pt);
            }
        }
    }
}

void CubicBezier::Flatten(std::vector<Vector2D>& out, float tolerance) const {
    CubicFlattenRecursive(out, *this, tolerance, 0);
    out.push_back(p3);
}

} // namespace koilo
