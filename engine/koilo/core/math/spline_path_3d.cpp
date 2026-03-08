// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/spline_path_3d.hpp>
#include <cmath>

namespace koilo {

void SplinePath3D::AddPoint(float x, float y, float z) {
    points_.push_back(Vector3D(x, y, z));
}

void SplinePath3D::Clear() { points_.clear(); }

int SplinePath3D::GetPointCount() const { return static_cast<int>(points_.size()); }

void SplinePath3D::SetLooping(bool loop) { looping_ = loop; }

bool SplinePath3D::GetLooping() const { return looping_; }

Vector3D SplinePath3D::GetPoint(int index) const {
    if (index < 0 || index >= static_cast<int>(points_.size()))
        return Vector3D(0.0f, 0.0f, 0.0f);
    return points_[index];
}

Vector3D SplinePath3D::Evaluate(float t) const {
    int n = static_cast<int>(points_.size());
    if (n == 0) return Vector3D(0.0f, 0.0f, 0.0f);
    if (n == 1) return points_[0];

    float maxT = looping_ ? static_cast<float>(n) : static_cast<float>(n - 1);

    if (looping_) {
        while (t < 0.0f) t += maxT;
        while (t >= maxT) t -= maxT;
    } else {
        if (t < 0.0f) t = 0.0f;
        if (t > maxT) t = maxT;
    }

    int seg = static_cast<int>(std::floor(t));
    float frac = t - static_cast<float>(seg);

    if (!looping_ && seg >= n - 1) {
        seg = n - 2;
        frac = 1.0f;
    }

    Vector3D p0 = GetWrappedPoint(seg - 1, n);
    Vector3D p1 = GetWrappedPoint(seg, n);
    Vector3D p2 = GetWrappedPoint(seg + 1, n);
    Vector3D p3 = GetWrappedPoint(seg + 2, n);

    return CatmullRom(p0, p1, p2, p3, frac);
}

Vector3D SplinePath3D::EvaluateTangent(float t) const {
    int n = static_cast<int>(points_.size());
    if (n < 2) return Vector3D(0.0f, 0.0f, 1.0f);

    float maxT = looping_ ? static_cast<float>(n) : static_cast<float>(n - 1);

    if (looping_) {
        while (t < 0.0f) t += maxT;
        while (t >= maxT) t -= maxT;
    } else {
        if (t < 0.0f) t = 0.0f;
        if (t > maxT) t = maxT;
    }

    int seg = static_cast<int>(std::floor(t));
    float frac = t - static_cast<float>(seg);

    if (!looping_ && seg >= n - 1) {
        seg = n - 2;
        frac = 1.0f;
    }

    Vector3D p0 = GetWrappedPoint(seg - 1, n);
    Vector3D p1 = GetWrappedPoint(seg, n);
    Vector3D p2 = GetWrappedPoint(seg + 1, n);
    Vector3D p3 = GetWrappedPoint(seg + 2, n);

    return CatmullRomDerivative(p0, p1, p2, p3, frac);
}

Vector3D SplinePath3D::GetWrappedPoint(int idx, int n) const {
    if (looping_) {
        idx = ((idx % n) + n) % n;
        return points_[idx];
    }
    if (idx < 0) return points_[0];
    if (idx >= n) return points_[n - 1];
    return points_[idx];
}

// q(t) = 0.5 * ((2*P1) + (-P0+P2)*t + (2*P0-5*P1+4*P2-P3)*t² + (-P0+3*P1-3*P2+P3)*t³)
Vector3D SplinePath3D::CatmullRom(const Vector3D& p0, const Vector3D& p1,
                                   const Vector3D& p2, const Vector3D& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return Vector3D(
        0.5f * ((2.0f*p1.X) + (-p0.X+p2.X)*t + (2.0f*p0.X-5.0f*p1.X+4.0f*p2.X-p3.X)*t2 + (-p0.X+3.0f*p1.X-3.0f*p2.X+p3.X)*t3),
        0.5f * ((2.0f*p1.Y) + (-p0.Y+p2.Y)*t + (2.0f*p0.Y-5.0f*p1.Y+4.0f*p2.Y-p3.Y)*t2 + (-p0.Y+3.0f*p1.Y-3.0f*p2.Y+p3.Y)*t3),
        0.5f * ((2.0f*p1.Z) + (-p0.Z+p2.Z)*t + (2.0f*p0.Z-5.0f*p1.Z+4.0f*p2.Z-p3.Z)*t2 + (-p0.Z+3.0f*p1.Z-3.0f*p2.Z+p3.Z)*t3)
    );
}

// q'(t) = 0.5 * ((-P0+P2) + 2*(2*P0-5*P1+4*P2-P3)*t + 3*(-P0+3*P1-3*P2+P3)*t²)
Vector3D SplinePath3D::CatmullRomDerivative(const Vector3D& p0, const Vector3D& p1,
                                              const Vector3D& p2, const Vector3D& p3, float t) {
    float t2 = t * t;
    return Vector3D(
        0.5f * ((-p0.X+p2.X) + 2.0f*(2.0f*p0.X-5.0f*p1.X+4.0f*p2.X-p3.X)*t + 3.0f*(-p0.X+3.0f*p1.X-3.0f*p2.X+p3.X)*t2),
        0.5f * ((-p0.Y+p2.Y) + 2.0f*(2.0f*p0.Y-5.0f*p1.Y+4.0f*p2.Y-p3.Y)*t + 3.0f*(-p0.Y+3.0f*p1.Y-3.0f*p2.Y+p3.Y)*t2),
        0.5f * ((-p0.Z+p2.Z) + 2.0f*(2.0f*p0.Z-5.0f*p1.Z+4.0f*p2.Z-p3.Z)*t + 3.0f*(-p0.Z+3.0f*p1.Z-3.0f*p2.Z+p3.Z)*t2)
    );
}

} // namespace koilo
