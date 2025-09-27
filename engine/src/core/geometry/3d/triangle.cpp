#include <ptx/core/geometry/3d/triangle.hpp>

Triangle3D::Triangle3D() : p1(), p2(), p3() {}

Triangle3D::Triangle3D(const Vector3D& v1, const Vector3D& v2, const Vector3D& v3)
    : p1(v1), p2(v2), p3(v3) {}

float Triangle3D::GetArea() const {
    // The magnitude of the cross product of two edge vectors is twice the area.
    return 0.5f * (p2 - p1).CrossProduct(p3 - p1).Magnitude();
}

Vector3D Triangle3D::GetNormal() const {
    Vector3D edge1 = p2 - p1;
    Vector3D edge2 = p3 - p1;
    return edge1.CrossProduct(edge2).UnitSphere();
}

Vector3D Triangle3D::GetCentroid() const {
    return (p1 + p2 + p3) / 3.0f;
}

Vector3D Triangle3D::ClosestPoint(const Vector3D& p) const {
    // Implementation of "Real-Time Collision Detection" by Christer Ericson, chapter 5.1.5
    Vector3D edge0 = p2 - p1;
    Vector3D edge1 = p3 - p1;
    Vector3D v0 = p1 - p;

    float a = edge0.DotProduct(edge0);
    float b = edge0.DotProduct(edge1);
    float c = edge1.DotProduct(edge1);
    float d = edge0.DotProduct(v0);
    float e = edge1.DotProduct(v0);

    float det = a * c - b * b;
    float s = b * e - c * d;
    float t = b * d - a * e;

    if (s + t <= det) {
        if (s < 0.0f) {
            if (t < 0.0f) { // Region 4
                if (d < 0.0f) {
                    s = (-d >= a) ? 1.0f : -d / a;
                    t = 0.0f;
                } else {
                    s = 0.0f;
                    t = (e >= 0.0f) ? 0.0f : (-e >= c ? 1.0f : -e / c);
                }
            } else { // Region 3
                s = 0.0f;
                t = (e >= 0.0f) ? 0.0f : (-e >= c ? 1.0f : -e / c);
            }
        } else if (t < 0.0f) { // Region 5
            s = (d >= 0.0f) ? 0.0f : (-d >= a ? 1.0f : -d / a);
            t = 0.0f;
        } else { // Region 0
            float invDet = 1.0f / det;
            s *= invDet;
            t *= invDet;
        }
    } else {
        if (s < 0.0f) { // Region 2
            float tmp0 = b + d;
            float tmp1 = c + e;
            if (tmp1 > tmp0) {
                float numer = tmp1 - tmp0;
                float denom = a - 2.0f * b + c;
                s = (numer >= denom) ? 1.0f : numer / denom;
                t = 1.0f - s;
            } else {
                s = 0.0f;
                t = (tmp1 <= 0.0f) ? 1.0f : (e >= 0.0f ? 0.0f : -e / c);
            }
        } else if (t < 0.0f) { // Region 6
            float tmp0 = b + e;
            float tmp1 = a + d;
            if (tmp1 > tmp0) {
                float numer = tmp1 - tmp0;
                float denom = a - 2.0f * b + c;
                t = (numer >= denom) ? 1.0f : numer / denom;
                s = 1.0f - t;
            } else {
                t = 0.0f;
                s = (tmp1 <= 0.0f) ? 1.0f : (d >= 0.0f ? 0.0f : -d / a);
            }
        } else { // Region 1
            float numer = (c + e) - (b + d);
            if (numer <= 0.0f) {
                s = 0.0f;
            } else {
                float denom = a - 2.0f * b + c;
                s = (numer >= denom) ? 1.0f : numer / denom;
            }
            t = 1.0f - s;
        }
    }
    return p1 + edge0 * s + edge1 * t;
}
