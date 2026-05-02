// SPDX-License-Identifier: GPL-3.0-or-later
#include "epa.hpp"
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <limits>

namespace koilo {

namespace {

constexpr float kEpaEps = 1e-7f;

inline float Dot(const Vector3D& a, const Vector3D& b) { return a.X*b.X + a.Y*b.Y + a.Z*b.Z; }

struct Face {
    int v[3];          ///< Indices into polytope vertex list.
    Vector3D normal;   ///< Outward unit normal (pointing away from origin).
    float distance;    ///< Distance from origin to face plane (along normal).
};

bool ComputeFace(const std::vector<SupportPoint>& verts, int i0, int i1, int i2, Face& out) {
    const Vector3D& a = verts[i0].point;
    const Vector3D& b = verts[i1].point;
    const Vector3D& c = verts[i2].point;
    Vector3D n = (b - a).CrossProduct(c - a);
    const float len = n.Magnitude();
    if (len < kEpaEps) return false;
    n = n * (1.0f / len);
    float d = Dot(n, a);
    if (d < 0.0f) {
        // Flip so normal points outward (away from origin which is inside).
        n = n * -1.0f;
        std::swap(out.v[1], out.v[2]); // not used here; we re-set below
        d = -d;
        out.v[0] = i0; out.v[1] = i2; out.v[2] = i1;
    } else {
        out.v[0] = i0; out.v[1] = i1; out.v[2] = i2;
    }
    out.normal = n;
    out.distance = d;
    return true;
}

// Builds a tetrahedron from a smaller GJK simplex when needed.
bool BuildInitialTetrahedron(const IShape& a, const BodyPose& poseA,
                             const IShape& b, const BodyPose& poseB,
                             const GjkResult& gjk,
                             std::vector<SupportPoint>& verts) {
    verts.clear();
    for (int i = 0; i < gjk.simplexCount; ++i) verts.push_back(gjk.simplex[i]);
    if (verts.size() == 4) return true;

    // Strategy: extend along basis directions / face normals to reach 4 affinely independent points.
    static const Vector3D basis[6] = {
        Vector3D(1,0,0), Vector3D(-1,0,0),
        Vector3D(0,1,0), Vector3D(0,-1,0),
        Vector3D(0,0,1), Vector3D(0,0,-1),
    };

    auto addSupportAlong = [&](const Vector3D& dir) -> bool {
        if (Dot(dir, dir) < kEpaEps) return false;
        SupportPoint sp = MinkowskiSupport(a, poseA, b, poseB, dir);
        for (auto& v : verts) {
            if ((v.point - sp.point).Magnitude() < kEpaEps) return false;
        }
        verts.push_back(sp);
        return true;
    };

    if (verts.size() == 1) {
        for (int i = 0; i < 6 && verts.size() < 2; ++i) addSupportAlong(basis[i]);
    }
    if (verts.size() == 2) {
        const Vector3D edge = verts[1].point - verts[0].point;
        for (int i = 0; i < 6 && verts.size() < 3; ++i) {
            const Vector3D perp = basis[i] - edge * (Dot(basis[i], edge) / std::max(Dot(edge, edge), kEpaEps));
            addSupportAlong(perp);
        }
    }
    if (verts.size() == 3) {
        const Vector3D n = (verts[1].point - verts[0].point).CrossProduct(verts[2].point - verts[0].point);
        addSupportAlong(n);
        if (verts.size() == 3) addSupportAlong(n * -1.0f);
    }
    return verts.size() == 4;
}

void MakeInitialFaces(const std::vector<SupportPoint>& verts, std::vector<Face>& faces) {
    faces.clear();
    Face f;
    if (ComputeFace(verts, 0, 1, 2, f)) faces.push_back(f);
    if (ComputeFace(verts, 0, 2, 3, f)) faces.push_back(f);
    if (ComputeFace(verts, 0, 3, 1, f)) faces.push_back(f);
    if (ComputeFace(verts, 1, 3, 2, f)) faces.push_back(f);
}

void BarycentricOnFace(const Vector3D& a, const Vector3D& b, const Vector3D& c,
                       const Vector3D& p, float& u, float& v, float& w) {
    const Vector3D v0 = b - a, v1 = c - a, v2 = p - a;
    const float d00 = Dot(v0, v0);
    const float d01 = Dot(v0, v1);
    const float d11 = Dot(v1, v1);
    const float d20 = Dot(v2, v0);
    const float d21 = Dot(v2, v1);
    const float denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < kEpaEps) { u = 1; v = 0; w = 0; return; }
    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0f - v - w;
}

} // namespace

EpaResult Epa(const IShape& a, const BodyPose& poseA,
              const IShape& b, const BodyPose& poseB,
              const GjkResult& gjk,
              int maxIterations,
              float tolerance) {
    EpaResult r;
    std::vector<SupportPoint> verts;
    if (!BuildInitialTetrahedron(a, poseA, b, poseB, gjk, verts)) {
        // Degenerate (touching contact, single shared support). Zero-depth fallback.
        if (!verts.empty()) {
            r.ok = true;
            r.depth = 0.0f;
            r.normal = Vector3D(0, 1, 0);
            r.witnessA = verts[0].supportA;
            r.witnessB = verts[0].supportB;
        }
        return r;
    }

    std::vector<Face> faces;
    MakeInitialFaces(verts, faces);
    if (faces.size() < 4) return r;

    for (int iter = 0; iter < maxIterations; ++iter) {
        r.iterations = iter + 1;

        // Pick face nearest origin.
        int bestIdx = 0;
        float bestDist = faces[0].distance;
        for (std::size_t i = 1; i < faces.size(); ++i) {
            if (faces[i].distance < bestDist) { bestDist = faces[i].distance; bestIdx = static_cast<int>(i); }
        }
        const Face best = faces[bestIdx];

        // Support along the face normal.
        const SupportPoint sp = MinkowskiSupport(a, poseA, b, poseB, best.normal);
        const float supportDist = Dot(sp.point, best.normal);
        if (supportDist - bestDist < tolerance) {
            // Converged. Recover witness via barycentrics of origin projection.
            const Vector3D proj = best.normal * bestDist;
            float u, v, w;
            BarycentricOnFace(verts[best.v[0]].point, verts[best.v[1]].point, verts[best.v[2]].point,
                              proj, u, v, w);
            r.ok = true;
            r.normal = best.normal * -1.0f;
            r.depth = bestDist;
            r.witnessA = verts[best.v[0]].supportA * u + verts[best.v[1]].supportA * v + verts[best.v[2]].supportA * w;
            r.witnessB = verts[best.v[0]].supportB * u + verts[best.v[1]].supportB * v + verts[best.v[2]].supportB * w;
            return r;
        }

        // Expand polytope: remove faces visible from new vertex; rebuild silhouette.
        const int newIdx = static_cast<int>(verts.size());
        verts.push_back(sp);

        struct Edge { int a, b; };
        std::vector<Edge> silhouette;
        std::vector<Face> kept;
        kept.reserve(faces.size());

        auto pushEdge = [&](int ea, int eb) {
            for (auto it = silhouette.begin(); it != silhouette.end(); ++it) {
                if (it->a == eb && it->b == ea) { silhouette.erase(it); return; }
            }
            silhouette.push_back({ea, eb});
        };

        for (const Face& f : faces) {
            if (Dot(f.normal, sp.point - verts[f.v[0]].point) > 0.0f) {
                // Face is visible; its edges contribute to silhouette.
                pushEdge(f.v[0], f.v[1]);
                pushEdge(f.v[1], f.v[2]);
                pushEdge(f.v[2], f.v[0]);
            } else {
                kept.push_back(f);
            }
        }
        if (silhouette.empty()) {
            // Numerical edge case - converge with current best.
            const Vector3D proj = best.normal * bestDist;
            float u, v, w;
            BarycentricOnFace(verts[best.v[0]].point, verts[best.v[1]].point, verts[best.v[2]].point,
                              proj, u, v, w);
            r.ok = true;
            r.normal = best.normal * -1.0f;
            r.depth = bestDist;
            r.witnessA = verts[best.v[0]].supportA * u + verts[best.v[1]].supportA * v + verts[best.v[2]].supportA * w;
            r.witnessB = verts[best.v[0]].supportB * u + verts[best.v[1]].supportB * v + verts[best.v[2]].supportB * w;
            return r;
        }
        for (const Edge& e : silhouette) {
            Face nf;
            if (ComputeFace(verts, e.a, e.b, newIdx, nf)) kept.push_back(nf);
        }
        faces.swap(kept);
        if (faces.empty()) return r;
    }
    return r;
}

} // namespace koilo
