// SPDX-License-Identifier: GPL-3.0-or-later
#include "gjk.hpp"
#include <cmath>
#include <algorithm>

namespace koilo {

namespace {

constexpr float kEps = 1e-6f;

inline float Dot(const Vector3D& a, const Vector3D& b) { return a.X*b.X + a.Y*b.Y + a.Z*b.Z; }
inline Vector3D TripleCross(const Vector3D& a, const Vector3D& b, const Vector3D& c) {
    // (a × b) × c
    return b * Dot(c, a) - a * Dot(c, b);
}

// Shifts the simplex so simplex[0..n-1] are the kept points, with the most
// recently added support at simplex[count-1].
inline void Set(SupportPoint* dst, const SupportPoint& a) { dst[0] = a; }

// Line: simplex contains [b, a]. a is most recent. Origin direction toward AO.
bool DoLine(SupportPoint* simplex, int& count, Vector3D& dir) {
    const Vector3D a = simplex[1].point;
    const Vector3D b = simplex[0].point;
    const Vector3D ab = b - a;
    const Vector3D ao = a * -1.0f;
    if (Dot(ab, ao) > 0.0f) {
        dir = TripleCross(ab, ao, ab);
        if (Dot(dir, dir) < kEps) {
            // Origin lies on the line; pick any perpendicular.
            dir = (std::fabs(ab.X) > std::fabs(ab.Y)) ? Vector3D(-ab.Y, ab.X, 0) : Vector3D(0, -ab.Z, ab.Y);
        }
    } else {
        Set(simplex, simplex[1]);
        count = 1;
        dir = ao;
    }
    return false;
}

// Triangle: simplex = [c, b, a]. a most recent.
bool DoTriangle(SupportPoint* simplex, int& count, Vector3D& dir) {
    const SupportPoint A = simplex[2];
    const SupportPoint B = simplex[1];
    const SupportPoint C = simplex[0];
    const Vector3D a = A.point, b = B.point, c = C.point;
    const Vector3D ab = b - a;
    const Vector3D ac = c - a;
    const Vector3D ao = a * -1.0f;
    const Vector3D abc = ab.CrossProduct(ac);

    // Edge AC region?
    if (Dot(abc.CrossProduct(ac), ao) > 0.0f) {
        if (Dot(ac, ao) > 0.0f) {
            simplex[0] = C; simplex[1] = A;
            count = 2;
            dir = TripleCross(ac, ao, ac);
            return false;
        }
        // Reduce to edge AB
        simplex[0] = B; simplex[1] = A;
        count = 2;
        return DoLine(simplex, count, dir);
    }
    // Edge AB region?
    if (Dot(ab.CrossProduct(abc), ao) > 0.0f) {
        simplex[0] = B; simplex[1] = A;
        count = 2;
        return DoLine(simplex, count, dir);
    }
    // Above or below triangle.
    if (Dot(abc, ao) > 0.0f) {
        // Above (in +abc direction). Keep order C,B,A.
        dir = abc;
    } else {
        // Below: flip winding to keep abc the outward normal next iteration.
        simplex[0] = B; simplex[1] = C; simplex[2] = A;
        dir = abc * -1.0f;
    }
    if (Dot(dir, dir) < kEps) {
        // Origin lies in the triangle plane. Use either side.
        dir = abc;
    }
    return false;
}

// Tetrahedron: simplex = [d, c, b, a]. a most recent. Returns true if origin enclosed.
bool DoTetrahedron(SupportPoint* simplex, int& count, Vector3D& dir) {
    const SupportPoint A = simplex[3];
    const SupportPoint B = simplex[2];
    const SupportPoint C = simplex[1];
    const SupportPoint D = simplex[0];
    const Vector3D a = A.point, b = B.point, c = C.point, d = D.point;
    const Vector3D ao = a * -1.0f;
    const Vector3D ab = b - a, ac = c - a, ad = d - a;
    const Vector3D abc = ab.CrossProduct(ac);
    const Vector3D acd = ac.CrossProduct(ad);
    const Vector3D adb = ad.CrossProduct(ab);

    // Origin must be on the "inside" of every face (opposite side from D, B, C respectively).
    // Each normal here points outward away from the opposite vertex.
    if (Dot(abc, ao) > 0.0f) {
        simplex[0] = C; simplex[1] = B; simplex[2] = A;
        count = 3;
        return DoTriangle(simplex, count, dir);
    }
    if (Dot(acd, ao) > 0.0f) {
        simplex[0] = D; simplex[1] = C; simplex[2] = A;
        count = 3;
        return DoTriangle(simplex, count, dir);
    }
    if (Dot(adb, ao) > 0.0f) {
        simplex[0] = B; simplex[1] = D; simplex[2] = A;
        count = 3;
        return DoTriangle(simplex, count, dir);
    }
    return true;
}

bool DoSimplex(SupportPoint* simplex, int& count, Vector3D& dir) {
    if (count == 2) return DoLine(simplex, count, dir);
    if (count == 3) return DoTriangle(simplex, count, dir);
    if (count == 4) return DoTetrahedron(simplex, count, dir);
    return false;
}

// Witness points: triangle (count=3) closest-point barycentrics.
void TriangleWitness(const SupportPoint& A, const SupportPoint& B, const SupportPoint& C,
                     Vector3D& outA, Vector3D& outB, float& outDistance) {
    // Closest point on triangle ABC to origin.
    const Vector3D a = A.point, b = B.point, c = C.point;
    const Vector3D ab = b - a, ac = c - a;
    const Vector3D ap = a * -1.0f;
    const float d1 = Dot(ab, ap), d2 = Dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) {
        outA = A.supportA; outB = A.supportB;
        outDistance = a.Magnitude(); return;
    }
    const Vector3D bp = b * -1.0f;
    const float d3 = Dot(ab, bp), d4 = Dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) {
        outA = B.supportA; outB = B.supportB;
        outDistance = b.Magnitude(); return;
    }
    const float vc = d1*d4 - d3*d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        const float v = d1 / (d1 - d3);
        outA = A.supportA + (B.supportA - A.supportA) * v;
        outB = A.supportB + (B.supportB - A.supportB) * v;
        outDistance = (a + ab * v).Magnitude(); return;
    }
    const Vector3D cp = c * -1.0f;
    const float d5 = Dot(ab, cp), d6 = Dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) {
        outA = C.supportA; outB = C.supportB;
        outDistance = c.Magnitude(); return;
    }
    const float vb = d5*d2 - d1*d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        const float w = d2 / (d2 - d6);
        outA = A.supportA + (C.supportA - A.supportA) * w;
        outB = A.supportB + (C.supportB - A.supportB) * w;
        outDistance = (a + ac * w).Magnitude(); return;
    }
    const float va = d3*d6 - d5*d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        outA = B.supportA + (C.supportA - B.supportA) * w;
        outB = B.supportB + (C.supportB - B.supportB) * w;
        outDistance = (b + (c - b) * w).Magnitude(); return;
    }
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    outA = A.supportA + (B.supportA - A.supportA) * v + (C.supportA - A.supportA) * w;
    outB = A.supportB + (B.supportB - A.supportB) * v + (C.supportB - A.supportB) * w;
    outDistance = (a + ab * v + ac * w).Magnitude();
}

void LineWitness(const SupportPoint& A, const SupportPoint& B,
                 Vector3D& outA, Vector3D& outB, float& outDistance) {
    const Vector3D ab = B.point - A.point;
    const float denom = Dot(ab, ab);
    if (denom < kEps) {
        outA = A.supportA; outB = A.supportB;
        outDistance = A.point.Magnitude(); return;
    }
    float t = -Dot(A.point, ab) / denom;
    if (t < 0) t = 0;
    else if (t > 1) t = 1;
    outA = A.supportA + (B.supportA - A.supportA) * t;
    outB = A.supportB + (B.supportB - A.supportB) * t;
    outDistance = (A.point + ab * t).Magnitude();
}

} // namespace

SupportPoint MinkowskiSupport(const IShape& a, const BodyPose& poseA,
                              const IShape& b, const BodyPose& poseB,
                              const Vector3D& d) {
    SupportPoint sp;
    sp.supportA = a.Support(d, poseA);
    sp.supportB = b.Support(d * -1.0f, poseB);
    sp.point = sp.supportA - sp.supportB;
    return sp;
}

GjkResult Gjk(const IShape& a, const BodyPose& poseA,
              const IShape& b, const BodyPose& poseB,
              int maxIterations) {
    GjkResult r;
    Vector3D dir(1, 0, 0);
    SupportPoint simplex[4];
    int count = 0;

    SupportPoint first = MinkowskiSupport(a, poseA, b, poseB, dir);
    simplex[0] = first;
    count = 1;
    dir = first.point * -1.0f;
    if (Dot(dir, dir) < kEps) dir = Vector3D(1, 0, 0);

    for (int iter = 0; iter < maxIterations; ++iter) {
        r.iterations = iter + 1;
        const SupportPoint p = MinkowskiSupport(a, poseA, b, poseB, dir);
        // Did we pass the origin?
        if (Dot(p.point, dir) < 0.0f) {
            // Separation. Compute closest points from current simplex (plus p? No, exclude - it didn't help).
            r.intersect = false;
            r.simplexCount = count;
            for (int i = 0; i < count; ++i) r.simplex[i] = simplex[i];
            if (count == 1) {
                r.closestA = simplex[0].supportA;
                r.closestB = simplex[0].supportB;
                r.distance = simplex[0].point.Magnitude();
            } else if (count == 2) {
                LineWitness(simplex[1], simplex[0], r.closestA, r.closestB, r.distance);
            } else if (count == 3) {
                TriangleWitness(simplex[2], simplex[1], simplex[0], r.closestA, r.closestB, r.distance);
            } else {
                r.closestA = simplex[count - 1].supportA;
                r.closestB = simplex[count - 1].supportB;
                r.distance = simplex[count - 1].point.Magnitude();
            }
            return r;
        }
        // Duplicate support (no progress).
        bool duplicate = false;
        for (int i = 0; i < count; ++i) {
            if ((simplex[i].point - p.point).Magnitude() < kEps) { duplicate = true; break; }
        }
        if (duplicate) {
            r.intersect = false;
            r.simplexCount = count;
            for (int i = 0; i < count; ++i) r.simplex[i] = simplex[i];
            if (count == 1) {
                r.closestA = simplex[0].supportA; r.closestB = simplex[0].supportB;
                r.distance = simplex[0].point.Magnitude();
            } else if (count == 2) {
                LineWitness(simplex[1], simplex[0], r.closestA, r.closestB, r.distance);
            } else if (count == 3) {
                TriangleWitness(simplex[2], simplex[1], simplex[0], r.closestA, r.closestB, r.distance);
            }
            return r;
        }
        simplex[count++] = p;
        if (DoSimplex(simplex, count, dir)) {
            r.intersect = true;
            r.simplexCount = count;
            for (int i = 0; i < count; ++i) r.simplex[i] = simplex[i];
            return r;
        }
        if (Dot(dir, dir) < kEps) {
            // Origin lies on a feature; treat as touching contact.
            r.intersect = true;
            r.simplexCount = count;
            for (int i = 0; i < count; ++i) r.simplex[i] = simplex[i];
            return r;
        }
    }
    // Iteration cap; conservative no-overlap.
    r.intersect = false;
    r.simplexCount = count;
    for (int i = 0; i < count; ++i) r.simplex[i] = simplex[i];
    if (count >= 1) {
        r.closestA = simplex[count - 1].supportA;
        r.closestB = simplex[count - 1].supportB;
        r.distance = simplex[count - 1].point.Magnitude();
    }
    return r;
}

} // namespace koilo
