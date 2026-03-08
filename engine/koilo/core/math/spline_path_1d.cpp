// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/spline_path_1d.hpp>
#include <cmath>

namespace koilo {

void SplinePath1D::AddPoint(float value) { values_.push_back(value); }

void SplinePath1D::Clear() { values_.clear(); }

int SplinePath1D::GetPointCount() const { return static_cast<int>(values_.size()); }

void SplinePath1D::SetLooping(bool loop) { looping_ = loop; }

bool SplinePath1D::GetLooping() const { return looping_; }

void SplinePath1D::SetAngleMode(bool angle) {
    angleMode_ = angle;
    if (angle) Unwrap();
}

float SplinePath1D::GetPoint(int index) const {
    if (index < 0 || index >= static_cast<int>(values_.size())) return 0.0f;
    if (angleMode_ && !unwrapped_.empty())
        return unwrapped_[index];
    return values_[index];
}

float SplinePath1D::Evaluate(float t) const {
    const auto& pts = (angleMode_ && !unwrapped_.empty()) ? unwrapped_ : values_;
    int n = static_cast<int>(pts.size());
    if (n == 0) return 0.0f;
    if (n == 1) return WrapResult(pts[0]);

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

    float p0 = GetWrapped(seg - 1, pts);
    float p1 = GetWrapped(seg, pts);
    float p2 = GetWrapped(seg + 1, pts);
    float p3 = GetWrapped(seg + 2, pts);

    // Ensure all 4 points are angularly continuous relative to P1
    if (angleMode_) {
        auto nearest = [](float val, float ref) {
            while (val - ref > 180.0f) val -= 360.0f;
            while (val - ref < -180.0f) val += 360.0f;
            return val;
        };
        p0 = nearest(p0, p1);
        p2 = nearest(p2, p1);
        p3 = nearest(p3, p2); // P3 relative to P2 for smooth derivative
    }

    float result = CatmullRom1D(p0, p1, p2, p3, frac);
    return WrapResult(result);
}

void SplinePath1D::Unwrap() {
    unwrapped_ = values_;
    for (size_t i = 1; i < unwrapped_.size(); i++) {
        float diff = unwrapped_[i] - unwrapped_[i - 1];
        while (diff > 180.0f) { unwrapped_[i] -= 360.0f; diff -= 360.0f; }
        while (diff < -180.0f) { unwrapped_[i] += 360.0f; diff += 360.0f; }
    }
}

float SplinePath1D::WrapResult(float v) const {
    if (!angleMode_) return v;
    while (v < 0.0f) v += 360.0f;
    while (v >= 360.0f) v -= 360.0f;
    return v;
}

float SplinePath1D::GetWrapped(int idx, const std::vector<float>& pts) const {
    int n = static_cast<int>(pts.size());
    if (looping_) {
        idx = ((idx % n) + n) % n;
        return pts[idx];
    }
    if (idx < 0) return pts[0];
    if (idx >= n) return pts[n - 1];
    return pts[idx];
}

float SplinePath1D::CatmullRom1D(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f*p1) + (-p0+p2)*t + (2.0f*p0-5.0f*p1+4.0f*p2-p3)*t2 + (-p0+3.0f*p1-3.0f*p2+p3)*t3);
}

} // namespace koilo
