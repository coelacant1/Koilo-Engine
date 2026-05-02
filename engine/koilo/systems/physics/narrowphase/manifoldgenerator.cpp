// SPDX-License-Identifier: GPL-3.0-or-later
#include "manifoldgenerator.hpp"
#include "gjk.hpp"
#include "epa.hpp"
#include "boxboxclip.hpp"
#include <koilo/systems/physics/shape/sphereshape.hpp>
#include <koilo/systems/physics/shape/boxshape.hpp>
#include <koilo/systems/physics/shape/capsuleshape.hpp>
#include <koilo/systems/physics/shape/planeshape.hpp>
#include <koilo/systems/physics/shape/trianglemeshshape.hpp>
#include <koilo/systems/physics/shape/heightfieldshape.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

namespace koilo {

namespace {

inline float Dot(const Vector3D& a, const Vector3D& b) { return a.X*b.X + a.Y*b.Y + a.Z*b.Z; }

BodyPose WorldPoseOf(const ColliderProxy& p, const BodyPose& bodyPose) {
    return bodyPose.Compose(p.localOffset);
}

bool SphereSphere(const SphereShape& sa, const BodyPose& pa,
                  const SphereShape& sb, const BodyPose& pb,
                  ContactManifold& out) {
    const Vector3D delta = pb.position - pa.position;
    const float distSq = Dot(delta, delta);
    const float radiusSum = sa.radius + sb.radius;
    if (distSq > radiusSum * radiusSum) return false;
    const float dist = std::sqrt(distSq);
    Vector3D normal = (dist > 1e-6f) ? delta * (1.0f / dist) : Vector3D(0, 1, 0);
    // Convention: normal points from B toward A.
    normal = normal * -1.0f;
    Contact c;
    c.normal = normal;
    c.depth = radiusSum - dist;
    c.point = pa.position + (normal * -1.0f) * sa.radius;
    c.featureId = 0;
    out.AddContact(c);
    return out.count > 0;
}

bool SpherePlane(const SphereShape& sa, const BodyPose& pa,
                 const PlaneShape& pl, const BodyPose& pb,
                 ContactManifold& out) {
    const Vector3D n = pl.NormalWorld(pb);
    const float d = pl.DistanceWorld(pb);
    const float dist = Dot(n, pa.position) - d;
    if (dist > sa.radius) return false;
    Contact c;
    // Normal points from B (plane) toward A (sphere).
    c.normal = n;
    c.depth = sa.radius - dist;
    c.point = pa.position - n * sa.radius;
    c.featureId = 0;
    out.AddContact(c);
    return out.count > 0;
}

bool SphereBox(const SphereShape& sa, const BodyPose& pa,
               const BoxShape& bx, const BodyPose& pb,
               ContactManifold& out) {
    // Bring sphere center into box-local space.
    const Vector3D rel = sa.radius == sa.radius ? (pa.position - pb.position) : Vector3D();
    const Vector3D local = pb.orientation.UnrotateVector(pa.position - pb.position);
    const Vector3D he = bx.halfExtents;
    Vector3D closest(
        std::max(-he.X, std::min(local.X, he.X)),
        std::max(-he.Y, std::min(local.Y, he.Y)),
        std::max(-he.Z, std::min(local.Z, he.Z))
    );
    const Vector3D delta = local - closest;
    const float distSq = Dot(delta, delta);
    if (distSq > sa.radius * sa.radius) return false;
    const float dist = std::sqrt(distSq);
    Vector3D normalLocal;
    if (dist > 1e-6f) {
        normalLocal = delta * (1.0f / dist);
    } else {
        // Sphere center inside box; pick smallest-penetration axis as normal.
        const float dx = he.X - std::fabs(local.X);
        const float dy = he.Y - std::fabs(local.Y);
        const float dz = he.Z - std::fabs(local.Z);
        if (dx <= dy && dx <= dz) normalLocal = Vector3D(local.X >= 0 ? 1.0f : -1.0f, 0, 0);
        else if (dy <= dz)        normalLocal = Vector3D(0, local.Y >= 0 ? 1.0f : -1.0f, 0);
        else                      normalLocal = Vector3D(0, 0, local.Z >= 0 ? 1.0f : -1.0f);
    }
    const Vector3D normalWorld = pb.orientation.RotateVector(normalLocal);
    Contact c;
    // Normal points from B (box) toward A (sphere).
    c.normal = normalWorld;
    c.depth = sa.radius - dist;
    c.point = pa.position - normalWorld * sa.radius;
    c.featureId = 0;
    out.AddContact(c);
    (void)rel;
    return out.count > 0;
}

bool ConvexPlane(const IShape& convex, const BodyPose& cPose,
                 const PlaneShape& pl, const BodyPose& pPose,
                 ContactManifold& out) {
    const Vector3D n = pl.NormalWorld(pPose);
    const float planeD = pl.DistanceWorld(pPose);
    // Sample support of convex along -n: the deepest point penetrating the plane.
    const Vector3D deepest = convex.Support(n * -1.0f, cPose);
    const float depth = planeD - Dot(n, deepest);
    if (depth <= 0.0f) return false;
    Contact c;
    // Normal points from B (plane) toward A (convex).
    c.normal = n;
    c.depth = depth;
    c.point = deepest;
    c.featureId = 0;
    out.AddContact(c);
    return out.count > 0;
}

bool GjkEpaPair(const IShape& sa, const BodyPose& pa,
                const IShape& sb, const BodyPose& pb,
                ContactManifold& out) {
    const GjkResult g = Gjk(sa, pa, sb, pb);
    if (!g.intersect) return false;
    const EpaResult e = Epa(sa, pa, sb, pb, g);
    if (!e.ok || e.depth <= 0.0f) return false;
    Contact c;
    // EPA normal already points from B toward A.
    c.normal = e.normal;
    c.depth = e.depth;
    c.point = (e.witnessA + e.witnessB) * 0.5f;
    c.featureId = 0;
    out.AddContact(c);
    return out.count > 0;
}

// Closest point on triangle (V0,V1,V2) to point P. Real-Time Collision
// Detection (Ericson) §5.1.5. All operands in the same frame.
inline Vector3D ClosestPointOnTriangle(const Vector3D& p,
                                       const Vector3D& a,
                                       const Vector3D& b,
                                       const Vector3D& c) {
    const Vector3D ab = b - a;
    const Vector3D ac = c - a;
    const Vector3D ap = p - a;
    const float d1 = Dot(ab, ap);
    const float d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    const Vector3D bp = p - b;
    const float d3 = Dot(ab, bp);
    const float d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        return a + ab * v;
    }

    const Vector3D cp = p - c;
    const float d5 = Dot(ab, cp);
    const float d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        return a + ac * w;
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w;
    }

    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    return a + ab * v + ac * w;
}

bool SphereTriangleMesh(const SphereShape& sphere, const BodyPose& pa,
                        const TriangleMeshShape& mesh, const BodyPose& pb,
                        ContactManifold& out) {
    const TriangleMeshData* data = mesh.Data();
    if (!data || data->Empty()) return false;

    // Transform sphere center into mesh-local space (cheap: 1 quat unrotate
    // + 1 vector subtract). Querying the BVH locally avoids transforming all
    // candidate triangles into world space.
    const Vector3D delta     = pa.position - pb.position;
    const Vector3D centerLoc = pb.orientation.UnrotateVector(delta);
    const float    r         = sphere.radius;
    const float    rSq       = r * r;

    const Vector3D rv(r, r, r);
    const AABB queryLocal(centerLoc - rv, centerLoc + rv);

    bool anyHit = false;
    data->bvh.Query(queryLocal, [&](std::uint32_t triIdx) {
        const std::uint32_t i0 = data->indices[3 * triIdx + 0];
        const std::uint32_t i1 = data->indices[3 * triIdx + 1];
        const std::uint32_t i2 = data->indices[3 * triIdx + 2];
        const Vector3D& v0 = data->vertices[i0];
        const Vector3D& v1 = data->vertices[i1];
        const Vector3D& v2 = data->vertices[i2];

        const Vector3D closestLoc = ClosestPointOnTriangle(centerLoc, v0, v1, v2);
        const Vector3D dLoc = centerLoc - closestLoc;
        const float distSq = Dot(dLoc, dLoc);
        if (distSq > rSq) return;

        const float dist = std::sqrt(distSq);

        // Local-space normal from triangle toward sphere. Fall back to
        // triangle face normal when the sphere center sits exactly on the
        // triangle (distSq ~ 0) so the impulse direction is well-defined.
        Vector3D normalLoc;
        if (dist > 1e-6f) {
            normalLoc = dLoc * (1.0f / dist);
        } else {
            const Vector3D e1 = v1 - v0;
            const Vector3D e2 = v2 - v0;
            const Vector3D fn(e1.Y*e2.Z - e1.Z*e2.Y,
                              e1.Z*e2.X - e1.X*e2.Z,
                              e1.X*e2.Y - e1.Y*e2.X);
            const float fnMag = std::sqrt(fn.X*fn.X + fn.Y*fn.Y + fn.Z*fn.Z);
            Vector3D nl = (fnMag > 1e-12f) ? fn * (1.0f / fnMag) : Vector3D(0, 1, 0);
            // Orient toward sphere centre so the contact normal pushes the
            // sphere away from the triangle regardless of triangle winding.
            if (Dot(nl, centerLoc - v0) < 0.0f) nl = nl * -1.0f;
            normalLoc = nl;
        }

        const Vector3D normalWorld  = pb.orientation.RotateVector(normalLoc);
        const Vector3D closestWorld = pb.position + pb.orientation.RotateVector(closestLoc);

        Contact c;
        // Normal points from B (mesh) toward A (sphere).
        c.normal    = normalWorld;
        c.depth     = r - dist;
        c.point     = closestWorld;
        c.featureId = static_cast<std::uint32_t>(triIdx);
        out.AddContact(c);
        anyHit = true;
    });

    return anyHit && out.count > 0;
}

// =============================================================================
// TriangleMesh helpers: segment/segment, segment/triangle, point-in-triangle,
// box-vs-triangle SAT, capsule/box vs TriangleMesh.
// =============================================================================

inline Vector3D Cross(const Vector3D& a, const Vector3D& b) {
    return Vector3D(a.Y*b.Z - a.Z*b.Y,
                    a.Z*b.X - a.X*b.Z,
                    a.X*b.Y - a.Y*b.X);
}

// Closest points between two segments (p1->p2) and (q1->q2). Real-Time
// Collision Detection (Ericson) §5.1.9. Returns (cs on first, ct on second).
inline void ClosestPtSegmentSegment(const Vector3D& p1, const Vector3D& p2,
                                    const Vector3D& q1, const Vector3D& q2,
                                    Vector3D& cs, Vector3D& ct) {
    const Vector3D d1 = p2 - p1;
    const Vector3D d2 = q2 - q1;
    const Vector3D r  = p1 - q1;
    const float a = Dot(d1, d1);
    const float e = Dot(d2, d2);
    const float f = Dot(d2, r);
    float s, t;
    if (a <= 1e-12f && e <= 1e-12f) { cs = p1; ct = q1; return; }
    if (a <= 1e-12f) {
        s = 0.0f;
        t = std::max(0.0f, std::min(1.0f, f / e));
    } else {
        const float c = Dot(d1, r);
        if (e <= 1e-12f) {
            t = 0.0f;
            s = std::max(0.0f, std::min(1.0f, -c / a));
        } else {
            const float b = Dot(d1, d2);
            const float denom = a*e - b*b;
            s = (denom != 0.0f) ? std::max(0.0f, std::min(1.0f, (b*f - c*e) / denom)) : 0.0f;
            t = (b*s + f) / e;
            if (t < 0.0f)      { t = 0.0f; s = std::max(0.0f, std::min(1.0f, -c / a)); }
            else if (t > 1.0f) { t = 1.0f; s = std::max(0.0f, std::min(1.0f, (b - c) / a)); }
        }
    }
    cs = p1 + d1 * s;
    ct = q1 + d2 * t;
}

// True if `p` is inside triangle (v0,v1,v2). Assumes `p` is on the triangle's
// plane (or near it). Uses unsigned barycentric test via cross products.
inline bool PointInTriangle(const Vector3D& p,
                            const Vector3D& v0, const Vector3D& v1, const Vector3D& v2) {
    const Vector3D u = v1 - v0;
    const Vector3D v = v2 - v0;
    const Vector3D w = p  - v0;
    const float uu = Dot(u, u);
    const float uv = Dot(u, v);
    const float vv = Dot(v, v);
    const float wu = Dot(w, u);
    const float wv = Dot(w, v);
    const float denom = uv * uv - uu * vv;
    if (std::fabs(denom) < 1e-20f) return false;
    const float invDenom = 1.0f / denom;
    const float s = (uv * wv - vv * wu) * invDenom;
    if (s < -1e-5f || s > 1.0f + 1e-5f) return false;
    const float t = (uv * wu - uu * wv) * invDenom;
    if (t < -1e-5f || (s + t) > 1.0f + 1e-5f) return false;
    return true;
}

// Closest point pair between segment (a,b) and triangle (v0,v1,v2).
// Returns distSq; writes closest points into cs/ct.
inline float SegmentTriangleClosest(const Vector3D& a, const Vector3D& b,
                                    const Vector3D& v0, const Vector3D& v1, const Vector3D& v2,
                                    Vector3D& cs, Vector3D& ct) {
    const Vector3D edge1 = v1 - v0;
    const Vector3D edge2 = v2 - v0;
    const Vector3D nUnnorm = Cross(edge1, edge2);
    const float    nMagSq  = Dot(nUnnorm, nUnnorm);

    float bestSq = std::numeric_limits<float>::max();
    cs = a; ct = v0;

    if (nMagSq > 1e-20f) {
        const float invMag = 1.0f / std::sqrt(nMagSq);
        const Vector3D n = nUnnorm * invMag;
        const float dA = Dot(n, a - v0);
        const float dB = Dot(n, b - v0);

        // (1) Plane-crossing case: if signs differ, segment cuts the plane.
        if (dA * dB <= 0.0f) {
            const float denom = dA - dB;
            if (std::fabs(denom) > 1e-20f) {
                const float tCross = dA / denom;
                if (tCross >= 0.0f && tCross <= 1.0f) {
                    const Vector3D P = a + (b - a) * tCross;
                    if (PointInTriangle(P, v0, v1, v2)) {
                        cs = P; ct = P;
                        return 0.0f;
                    }
                }
            }
        }

        // (2) Perpendicular-foot candidate: closest segment point to the
        // triangle plane, projected. Catches "segment passes over interior".
        // d(t) is linear; |d| minimised at t* = clamp(dA/(dA-dB), 0, 1).
        float tStar;
        const float diff = dA - dB;
        if (std::fabs(diff) > 1e-20f) {
            tStar = std::max(0.0f, std::min(1.0f, dA / diff));
        } else {
            tStar = 0.0f; // segment parallel to plane: any t has same |d|
        }
        const Vector3D Pt   = a + (b - a) * tStar;
        const float    dPt  = dA * (1.0f - tStar) + dB * tStar;
        const Vector3D foot = Pt - n * dPt;
        if (PointInTriangle(foot, v0, v1, v2)) {
            const float d2 = dPt * dPt;
            if (d2 < bestSq) { bestSq = d2; cs = Pt; ct = foot; }
        }
    }

    // (3) Endpoint-vs-triangle candidates.
    auto considerPoint = [&](const Vector3D& p) {
        const Vector3D q = ClosestPointOnTriangle(p, v0, v1, v2);
        const Vector3D d = p - q;
        const float d2 = Dot(d, d);
        if (d2 < bestSq) { bestSq = d2; cs = p; ct = q; }
    };
    considerPoint(a);
    considerPoint(b);

    // (4) Segment-vs-each-triangle-edge candidates.
    auto considerEdge = [&](const Vector3D& e0, const Vector3D& e1) {
        Vector3D s, t;
        ClosestPtSegmentSegment(a, b, e0, e1, s, t);
        const Vector3D d = s - t;
        const float d2 = Dot(d, d);
        if (d2 < bestSq) { bestSq = d2; cs = s; ct = t; }
    };
    considerEdge(v0, v1);
    considerEdge(v1, v2);
    considerEdge(v2, v0);

    return bestSq;
}

bool CapsuleTriangleMesh(const CapsuleShape& cap, const BodyPose& pa,
                         const TriangleMeshShape& mesh, const BodyPose& pb,
                         ContactManifold& out) {
    const TriangleMeshData* data = mesh.Data();
    if (!data || data->Empty()) return false;

    // Capsule segment endpoints in world, then in mesh-local.
    const Vector3D axisLocalCap(0.0f, cap.halfHeight, 0.0f);
    const Vector3D axisWorld    = pa.orientation.RotateVector(axisLocalCap);
    const Vector3D pTopWorld    = pa.position + axisWorld;
    const Vector3D pBotWorld    = pa.position - axisWorld;
    const Vector3D pTopLoc      = pb.orientation.UnrotateVector(pTopWorld - pb.position);
    const Vector3D pBotLoc      = pb.orientation.UnrotateVector(pBotWorld - pb.position);
    const float    r            = cap.radius;
    const float    rSq          = r * r;

    // Query AABB: segment AABB inflated by radius.
    Vector3D mn(std::min(pTopLoc.X, pBotLoc.X) - r,
                std::min(pTopLoc.Y, pBotLoc.Y) - r,
                std::min(pTopLoc.Z, pBotLoc.Z) - r);
    Vector3D mx(std::max(pTopLoc.X, pBotLoc.X) + r,
                std::max(pTopLoc.Y, pBotLoc.Y) + r,
                std::max(pTopLoc.Z, pBotLoc.Z) + r);
    const AABB queryLocal(mn, mx);

    bool anyHit = false;
    data->bvh.Query(queryLocal, [&](std::uint32_t triIdx) {
        const std::uint32_t i0 = data->indices[3 * triIdx + 0];
        const std::uint32_t i1 = data->indices[3 * triIdx + 1];
        const std::uint32_t i2 = data->indices[3 * triIdx + 2];
        const Vector3D& v0 = data->vertices[i0];
        const Vector3D& v1 = data->vertices[i1];
        const Vector3D& v2 = data->vertices[i2];

        Vector3D cs, ct;
        const float distSq = SegmentTriangleClosest(pBotLoc, pTopLoc, v0, v1, v2, cs, ct);
        if (distSq > rSq) return;

        Vector3D normalLoc;
        Vector3D contactPointLoc;
        float    contactDepth;

        if (distSq > 1e-12f) {
            // Standard separation case: closest pair is well-defined.
            const float dist = std::sqrt(distSq);
            normalLoc       = (cs - ct) * (1.0f / dist);
            contactDepth    = r - dist;
            contactPointLoc = ct;
        } else {
            // Penetration case: segment crosses triangle interior. The naive
            // r - dist = r underestimates depth because the segment extends
            // further beyond the plane than the closest-pair distance shows.
            // Use face-normal projection of the segment to recover the
            // correct minimum-translation depth.
            const Vector3D fn = Cross(v1 - v0, v2 - v0);
            const float    fm = std::sqrt(Dot(fn, fn));
            Vector3D nl = (fm > 1e-12f) ? fn * (1.0f / fm) : Vector3D(0, 1, 0);
            const float    dB = Dot(nl, pBotLoc - v0);
            const float    dT = Dot(nl, pTopLoc - v0);
            const float    dMin = std::min(dB, dT);
            const float    dMax = std::max(dB, dT);
            const float    pushPlus  = r - dMin;   // separate by moving along +nl
            const float    pushMinus = dMax + r;   // separate by moving along -nl
            if (pushPlus <= pushMinus) {
                contactDepth        = pushPlus;
                const Vector3D deep = (dB < dT) ? pBotLoc : pTopLoc;
                contactPointLoc     = deep - nl * dMin;
                normalLoc           = nl;
            } else {
                contactDepth        = pushMinus;
                const Vector3D deep = (dB > dT) ? pBotLoc : pTopLoc;
                contactPointLoc     = deep - nl * dMax;
                normalLoc           = nl * -1.0f;
            }
        }

        const Vector3D normalWorld  = pb.orientation.RotateVector(normalLoc);
        const Vector3D closestWorld = pb.position + pb.orientation.RotateVector(contactPointLoc);

        Contact c;
        // Normal points from B (mesh) toward A (capsule).
        c.normal    = normalWorld;
        c.depth     = contactDepth;
        c.point     = closestWorld;
        c.featureId = static_cast<std::uint32_t>(triIdx);
        out.AddContact(c);
        anyHit = true;
    });

    return anyHit && out.count > 0;
}

bool BoxTriangleMesh(const BoxShape& box, const BodyPose& pa,
                     const TriangleMeshShape& mesh, const BodyPose& pb,
                     ContactManifold& out) {
    const TriangleMeshData* data = mesh.Data();
    if (!data || data->Empty()) return false;

    // Box pose expressed in mesh-local frame.
    const Vector3D he       = box.halfExtents;
    const Vector3D boxC_loc = pb.orientation.UnrotateVector(pa.position - pb.position);
    // Box axes in mesh-local: rotate world basis by (mesh.q^-1 * box.q).
    const Vector3D u0_loc = pb.orientation.UnrotateVector(pa.orientation.RotateVector(Vector3D(1,0,0)));
    const Vector3D u1_loc = pb.orientation.UnrotateVector(pa.orientation.RotateVector(Vector3D(0,1,0)));
    const Vector3D u2_loc = pb.orientation.UnrotateVector(pa.orientation.RotateVector(Vector3D(0,0,1)));

    // Mesh-local AABB enclosing the OBB.
    const float ex = std::fabs(u0_loc.X)*he.X + std::fabs(u1_loc.X)*he.Y + std::fabs(u2_loc.X)*he.Z;
    const float ey = std::fabs(u0_loc.Y)*he.X + std::fabs(u1_loc.Y)*he.Y + std::fabs(u2_loc.Y)*he.Z;
    const float ez = std::fabs(u0_loc.Z)*he.X + std::fabs(u1_loc.Z)*he.Y + std::fabs(u2_loc.Z)*he.Z;
    const Vector3D extentVec(ex, ey, ez);
    const AABB queryLocal(boxC_loc - extentVec, boxC_loc + extentVec);

    // Box face axes (local): for face clipping, we need the 8 box vertices
    // in mesh-local. Compute once.
    Vector3D boxVerts[8];
    for (int i = 0; i < 8; ++i) {
        const float sx = (i & 1) ? he.X : -he.X;
        const float sy = (i & 2) ? he.Y : -he.Y;
        const float sz = (i & 4) ? he.Z : -he.Z;
        boxVerts[i] = boxC_loc + u0_loc * sx + u1_loc * sy + u2_loc * sz;
    }
    // 6 box face groups: each entry = {axis (outward-normal), vertex index list of size 4}.
    // Indices match the standard hex layout above.
    static const int faceVtx[6][4] = {
        {0,2,6,4}, // -X
        {1,5,7,3}, // +X
        {0,4,5,1}, // -Y
        {2,3,7,6}, // +Y
        {0,1,3,2}, // -Z
        {4,6,7,5}, // +Z
    };
    const Vector3D faceAxes[6] = {
        u0_loc * -1.0f, u0_loc, u1_loc * -1.0f, u1_loc, u2_loc * -1.0f, u2_loc
    };

    bool anyHit = false;

    auto projectBoxAlongAxis = [&](const Vector3D& axis, float& mn, float& mx) {
        const float c  = Dot(boxC_loc, axis);
        const float ex2 = std::fabs(Dot(u0_loc, axis))*he.X
                        + std::fabs(Dot(u1_loc, axis))*he.Y
                        + std::fabs(Dot(u2_loc, axis))*he.Z;
        mn = c - ex2;
        mx = c + ex2;
    };

    data->bvh.Query(queryLocal, [&](std::uint32_t triIdx) {
        const std::uint32_t i0 = data->indices[3 * triIdx + 0];
        const std::uint32_t i1 = data->indices[3 * triIdx + 1];
        const std::uint32_t i2 = data->indices[3 * triIdx + 2];
        const Vector3D& v0 = data->vertices[i0];
        const Vector3D& v1 = data->vertices[i1];
        const Vector3D& v2 = data->vertices[i2];

        const Vector3D e0 = v1 - v0;
        const Vector3D e1 = v2 - v1;
        const Vector3D e2 = v0 - v2;
        const Vector3D triN_unnorm = Cross(e0, v2 - v0); // = Cross(e0, -e2)
        const float    triN_magSq  = Dot(triN_unnorm, triN_unnorm);
        if (triN_magSq < 1e-20f) return; // degenerate (already filtered, but safe)
        const float    triN_invMag = 1.0f / std::sqrt(triN_magSq);
        const Vector3D triN = triN_unnorm * triN_invMag;

        // Build 13-axis SAT. Track minimum-overlap normalized axis.
        Vector3D bestAxis(0,0,0);
        float    bestOverlap = std::numeric_limits<float>::max();
        bool     anyAxis = false;

        auto tryAxis = [&](const Vector3D& axisRaw) -> bool {
            const float magSq = Dot(axisRaw, axisRaw);
            if (magSq < 1e-12f) return true; // skip degenerate
            const float invMag = 1.0f / std::sqrt(magSq);
            const Vector3D axis = axisRaw * invMag;

            float boxMn, boxMx;
            projectBoxAlongAxis(axis, boxMn, boxMx);
            const float t0 = Dot(v0, axis);
            const float t1 = Dot(v1, axis);
            const float t2 = Dot(v2, axis);
            const float triMn = std::min(t0, std::min(t1, t2));
            const float triMx = std::max(t0, std::max(t1, t2));

            if (boxMx < triMn || triMx < boxMn) return false; // separating axis
            const float overlap = std::min(boxMx - triMn, triMx - boxMn);
            if (overlap < bestOverlap) {
                bestOverlap = overlap;
                bestAxis    = axis;
                anyAxis     = true;
            }
            return true;
        };

        // 3 box face axes.
        if (!tryAxis(u0_loc)) return;
        if (!tryAxis(u1_loc)) return;
        if (!tryAxis(u2_loc)) return;
        // 1 triangle face axis.
        if (!tryAxis(triN)) return;
        // 9 cross-product axes.
        if (!tryAxis(Cross(u0_loc, e0))) return;
        if (!tryAxis(Cross(u0_loc, e1))) return;
        if (!tryAxis(Cross(u0_loc, e2))) return;
        if (!tryAxis(Cross(u1_loc, e0))) return;
        if (!tryAxis(Cross(u1_loc, e1))) return;
        if (!tryAxis(Cross(u1_loc, e2))) return;
        if (!tryAxis(Cross(u2_loc, e0))) return;
        if (!tryAxis(Cross(u2_loc, e1))) return;
        if (!tryAxis(Cross(u2_loc, e2))) return;
        if (!anyAxis) return;

        // Orient the SAT axis to point from triangle toward box (so the
        // contact normal pushes the box away from the triangle).
        Vector3D nLoc = bestAxis;
        const Vector3D triCent = (v0 + v1 + v2) * (1.0f / 3.0f);
        if (Dot(nLoc, boxC_loc - triCent) < 0.0f) nLoc = nLoc * -1.0f;

        // Contact-generation strategy:
        // (A) If the winning axis is the triangle face normal (within a
        //     tolerance), do face-clip-style multi-contact: project each
        //     incident-box-face vertex onto the triangle plane; emit one
        //     contact per vertex whose projection is inside the triangle.
        //     Then add triangle vertices that lie within the incident box
        //     face's lateral extent. This keeps a resting box stable.
        // (B) Otherwise (edge or box-face axis won): single contact at the
        //     deepest box-vertex along -nLoc / closest triangle point.
        const float faceAlign = std::fabs(Dot(nLoc, triN));
        const bool  useFaceClip = faceAlign > 0.95f;

        const Vector3D normalWorld = pb.orientation.RotateVector(nLoc);

        if (useFaceClip) {
            // Find the box face whose outward normal best matches -nLoc
            // (the face pressing into the triangle).
            int bestFace = 0;
            float bestDot = -std::numeric_limits<float>::max();
            for (int f = 0; f < 6; ++f) {
                const float d = Dot(faceAxes[f], nLoc * -1.0f);
                if (d > bestDot) { bestDot = d; bestFace = f; }
            }

            std::uint32_t emitted = 0;

            // (A1) Box face vertices projected onto triangle.
            for (int k = 0; k < 4 && emitted < 6; ++k) {
                const Vector3D& V = boxVerts[faceVtx[bestFace][k]];
                const float    s  = Dot(triN, V - v0);
                const Vector3D Vp = V - triN * s;
                if (!PointInTriangle(Vp, v0, v1, v2)) continue;
                const float depth = -Dot(nLoc, V - v0); // positive when penetrating
                if (depth <= 0.0f) continue;

                const Vector3D pointWorld = pb.position + pb.orientation.RotateVector(Vp);
                Contact c;
                c.normal    = normalWorld;
                c.depth     = depth;
                c.point     = pointWorld;
                c.featureId = (static_cast<std::uint32_t>(triIdx) << 4) | static_cast<std::uint32_t>(k);
                out.AddContact(c);
                ++emitted;
                anyHit = true;
            }

            // (A2) Triangle vertices that lie within the incident box face's
            // lateral extent (project tri vertex onto box face plane, test
            // against the two in-face axes' half-extents).
            // Determine the two in-face axes & their extents from bestFace.
            int axisIdx = bestFace / 2; // 0,1,2
            Vector3D inFaceA, inFaceB;
            float heA, heB;
            if (axisIdx == 0) { inFaceA = u1_loc; inFaceB = u2_loc; heA = he.Y; heB = he.Z; }
            else if (axisIdx == 1) { inFaceA = u0_loc; inFaceB = u2_loc; heA = he.X; heB = he.Z; }
            else { inFaceA = u0_loc; inFaceB = u1_loc; heA = he.X; heB = he.Y; }

            const Vector3D triVerts[3] = { v0, v1, v2 };
            for (int k = 0; k < 3 && emitted < 8; ++k) {
                const Vector3D& V  = triVerts[k];
                const Vector3D rel = V - boxC_loc;
                const float    a   = Dot(rel, inFaceA);
                const float    b   = Dot(rel, inFaceB);
                if (std::fabs(a) > heA + 1e-5f || std::fabs(b) > heB + 1e-5f) continue;
                // Penetration depth = -dot(nLoc, V - nearest-box-face-point).
                // Nearest box-face-point along nLoc is boxC_loc + bestFace's outward extent.
                const float boxFaceCenterAlongN = Dot(boxC_loc, nLoc) - bestOverlap; // approx
                (void)boxFaceCenterAlongN;
                // Simpler: depth = -dot(nLoc, V - boxC_loc) - he-along-(-nLoc).
                // Project boxC_loc onto -nLoc: extent along -nLoc =
                //   |dot(u0,nLoc)|*he.X + |dot(u1,nLoc)|*he.Y + |dot(u2,nLoc)|*he.Z.
                const float boxExtAlongN = std::fabs(Dot(u0_loc, nLoc))*he.X
                                         + std::fabs(Dot(u1_loc, nLoc))*he.Y
                                         + std::fabs(Dot(u2_loc, nLoc))*he.Z;
                const float depth = boxExtAlongN + Dot(nLoc, V - boxC_loc);
                if (depth <= 0.0f) continue;

                const Vector3D pointWorld = pb.position + pb.orientation.RotateVector(V);
                Contact c;
                c.normal    = normalWorld;
                c.depth     = depth;
                c.point     = pointWorld;
                c.featureId = (static_cast<std::uint32_t>(triIdx) << 4) | (4u + static_cast<std::uint32_t>(k));
                out.AddContact(c);
                ++emitted;
                anyHit = true;
            }

            if (emitted > 0) return;
            // Fall through to single-contact path if face-clip emitted nothing.
        }

        // Single-contact path: deepest box vertex along -nLoc as the contact
        // location; pair with closest triangle point for stability.
        Vector3D deepestV = boxVerts[0];
        float    minDot   = Dot(nLoc, boxVerts[0]);
        for (int k = 1; k < 8; ++k) {
            const float d = Dot(nLoc, boxVerts[k]);
            if (d < minDot) { minDot = d; deepestV = boxVerts[k]; }
        }
        const Vector3D triClosest = ClosestPointOnTriangle(deepestV, v0, v1, v2);
        const Vector3D pointLoc   = (deepestV + triClosest) * 0.5f;
        const Vector3D pointWorld = pb.position + pb.orientation.RotateVector(pointLoc);

        Contact c;
        c.normal    = normalWorld;
        c.depth     = bestOverlap;
        c.point     = pointWorld;
        c.featureId = static_cast<std::uint32_t>(triIdx) << 4;
        out.AddContact(c);
        anyHit = true;
    });

    return anyHit && out.count > 0;
}

// =============================================================================
// HeightfieldShape narrowphase helpers. Each function brings the dynamic
// shape into mesh-local, computes the cell range overlapping its AABB, and
// per cell synthesizes 2 triangles whose CCW winding gives a +Y face normal
// on flat terrain. Per-triangle math mirrors the TriangleMesh code
// (deliberately duplicated to avoid touching the proven mesh paths).
// =============================================================================

// Maps an axis-aligned X/Z range to inclusive cell indices in [0, dim-1].
// Returns false if the range is fully outside the heightfield extents.
inline bool CellRange(float lo, float hi, std::uint32_t dim, float cellSize,
                      std::uint32_t& i0, std::uint32_t& i1) {
    if (dim == 0 || cellSize <= 0.0f) return false;
    const float halfExtent = dim * 0.5f * cellSize;
    if (hi < -halfExtent - 1e-5f || lo > halfExtent + 1e-5f) return false;
    const float fLo = std::floor((lo + halfExtent) / cellSize);
    const float fHi = std::floor((hi + halfExtent) / cellSize);
    const float dimF = static_cast<float>(dim);
    const float c0   = std::max(0.0f, std::min(dimF - 1.0f, fLo));
    const float c1   = std::max(0.0f, std::min(dimF - 1.0f, fHi));
    i0 = static_cast<std::uint32_t>(c0);
    i1 = static_cast<std::uint32_t>(c1);
    return true;
}

// CCW winding for +Y normal on flat terrain.
//   t0 = (v00, v11, v10)
//   t1 = (v00, v01, v11)
inline void CellTriangles(const HeightfieldData& d, std::uint32_t i, std::uint32_t j,
                          Vector3D& v00, Vector3D& v10, Vector3D& v11, Vector3D& v01) {
    v00 = d.Vertex(i,   j  );
    v10 = d.Vertex(i+1, j  );
    v11 = d.Vertex(i+1, j+1);
    v01 = d.Vertex(i,   j+1);
}

// Reduce up to N candidate contacts to at most kMaxContacts (=4) using the
// "deepest + 3 farthest-spread" policy. Avoids the streaming AddContact path
// when many equal-depth candidates are generated (e.g. box on flat terrain).
inline void ReduceCandidatesToManifold(const std::vector<Contact>& cands,
                                       ContactManifold& out) {
    if (cands.empty()) return;
    if (cands.size() <= ContactManifold::kMaxContacts) {
        for (const Contact& c : cands) out.AddContact(c);
        return;
    }
    // 1) deepest
    std::size_t i0 = 0;
    for (std::size_t i = 1; i < cands.size(); ++i) {
        if (cands[i].depth > cands[i0].depth) i0 = i;
    }
    out.AddContact(cands[i0]);

    // 2) farthest from i0
    std::size_t i1 = (i0 == 0) ? 1 : 0;
    float bestD2 = -1.0f;
    for (std::size_t i = 0; i < cands.size(); ++i) {
        if (i == i0) continue;
        const Vector3D d = cands[i].point - cands[i0].point;
        const float d2 = Dot(d, d);
        if (d2 > bestD2) { bestD2 = d2; i1 = i; }
    }
    out.AddContact(cands[i1]);

    // 3) farthest from segment i0-i1 (max distance to the line through them)
    const Vector3D ab = cands[i1].point - cands[i0].point;
    const float ab2  = std::max(Dot(ab, ab), 1e-12f);
    std::size_t i2 = (i0 != 2 && i1 != 2) ? 2 : 0;
    float bestPerp2 = -1.0f;
    for (std::size_t i = 0; i < cands.size(); ++i) {
        if (i == i0 || i == i1) continue;
        const Vector3D ap = cands[i].point - cands[i0].point;
        const float t = Dot(ap, ab) / ab2;
        const Vector3D proj = ab * t;
        const Vector3D perp = ap - proj;
        const float p2 = Dot(perp, perp);
        if (p2 > bestPerp2) { bestPerp2 = p2; i2 = i; }
    }
    if (i2 != i0 && i2 != i1) out.AddContact(cands[i2]);

    // 4) maximize triangle-area off the previous 3 (signed-area squared in 3D
    // approximated via cross-magnitude sum from each candidate to the prior
    // triple).
    std::size_t i3 = i0;
    float bestArea = -1.0f;
    for (std::size_t i = 0; i < cands.size(); ++i) {
        if (i == i0 || i == i1 || (i2 != i0 && i2 != i1 && i == i2)) continue;
        const Vector3D p = cands[i].point;
        const Vector3D d1 = cands[i0].point - p;
        const Vector3D d2 = cands[i1].point - p;
        const Vector3D d3 = (i2 != i0 && i2 != i1) ? (cands[i2].point - p) : d2;
        const Vector3D x1 = Cross(d1, d2);
        const Vector3D x2 = Cross(d2, d3);
        const float    s  = Dot(x1, x1) + Dot(x2, x2);
        if (s > bestArea) { bestArea = s; i3 = i; }
    }
    if (i3 != i0 && i3 != i1 && i3 != i2) out.AddContact(cands[i3]);
}

// -- Sphere vs Heightfield ----------------------------------------------------
bool SphereHeightfield(const SphereShape& sphere, const BodyPose& pa,
                       const HeightfieldShape& hf, const BodyPose& pb,
                       ContactManifold& out) {
    const HeightfieldData* data = hf.Data();
    if (!data || data->Empty()) return false;

    const Vector3D delta     = pa.position - pb.position;
    const Vector3D centerLoc = pb.orientation.UnrotateVector(delta);
    const float    r         = sphere.radius;
    const float    rSq       = r * r;

    std::uint32_t i0, i1, j0, j1;
    if (!CellRange(centerLoc.X - r, centerLoc.X + r, data->widthCells, data->cellSize, i0, i1)) return false;
    if (!CellRange(centerLoc.Z - r, centerLoc.Z + r, data->depthCells, data->cellSize, j0, j1)) return false;

    bool anyHit = false;
    for (std::uint32_t j = j0; j <= j1; ++j) {
        for (std::uint32_t i = i0; i <= i1; ++i) {
            Vector3D v00, v10, v11, v01;
            CellTriangles(*data, i, j, v00, v10, v11, v01);
            const std::uint64_t cellLin = static_cast<std::uint64_t>(j) * data->widthCells + i;
            const Vector3D triV[2][3] = {
                { v00, v11, v10 },
                { v00, v01, v11 },
            };
            for (int t = 0; t < 2; ++t) {
                const Vector3D& a = triV[t][0];
                const Vector3D& b = triV[t][1];
                const Vector3D& c = triV[t][2];
                const Vector3D closestLoc = ClosestPointOnTriangle(centerLoc, a, b, c);
                const Vector3D dLoc = centerLoc - closestLoc;
                const float distSq = Dot(dLoc, dLoc);
                if (distSq > rSq) continue;
                const float dist = std::sqrt(distSq);
                Vector3D normalLoc;
                if (dist > 1e-6f) {
                    normalLoc = dLoc * (1.0f / dist);
                } else {
                    const Vector3D e1 = b - a;
                    const Vector3D e2 = c - a;
                    const Vector3D fn = Cross(e1, e2);
                    const float    fm = std::sqrt(Dot(fn, fn));
                    Vector3D nl = (fm > 1e-12f) ? fn * (1.0f / fm) : Vector3D(0,1,0);
                    if (Dot(nl, centerLoc - a) < 0.0f) nl = nl * -1.0f;
                    normalLoc = nl;
                }
                Contact ct;
                ct.normal    = pb.orientation.RotateVector(normalLoc);
                ct.depth     = r - dist;
                ct.point     = pb.position + pb.orientation.RotateVector(closestLoc);
                ct.featureId = (cellLin << 1) | static_cast<std::uint64_t>(t);
                out.AddContact(ct);
                anyHit = true;
            }
        }
    }
    return anyHit && out.count > 0;
}

// -- Capsule vs Heightfield ---------------------------------------------------
bool CapsuleHeightfield(const CapsuleShape& cap, const BodyPose& pa,
                        const HeightfieldShape& hf, const BodyPose& pb,
                        ContactManifold& out) {
    const HeightfieldData* data = hf.Data();
    if (!data || data->Empty()) return false;

    const Vector3D axisLocalCap(0.0f, cap.halfHeight, 0.0f);
    const Vector3D axisWorld = pa.orientation.RotateVector(axisLocalCap);
    const Vector3D pTopLoc   = pb.orientation.UnrotateVector((pa.position + axisWorld) - pb.position);
    const Vector3D pBotLoc   = pb.orientation.UnrotateVector((pa.position - axisWorld) - pb.position);
    const float    r         = cap.radius;
    const float    rSq       = r * r;

    const float minX = std::min(pTopLoc.X, pBotLoc.X) - r;
    const float maxX = std::max(pTopLoc.X, pBotLoc.X) + r;
    const float minZ = std::min(pTopLoc.Z, pBotLoc.Z) - r;
    const float maxZ = std::max(pTopLoc.Z, pBotLoc.Z) + r;

    std::uint32_t i0, i1, j0, j1;
    if (!CellRange(minX, maxX, data->widthCells, data->cellSize, i0, i1)) return false;
    if (!CellRange(minZ, maxZ, data->depthCells, data->cellSize, j0, j1)) return false;

    bool anyHit = false;
    for (std::uint32_t j = j0; j <= j1; ++j) {
        for (std::uint32_t i = i0; i <= i1; ++i) {
            Vector3D v00, v10, v11, v01;
            CellTriangles(*data, i, j, v00, v10, v11, v01);
            const std::uint64_t cellLin = static_cast<std::uint64_t>(j) * data->widthCells + i;
            const Vector3D triV[2][3] = {
                { v00, v11, v10 },
                { v00, v01, v11 },
            };
            for (int t = 0; t < 2; ++t) {
                const Vector3D& a = triV[t][0];
                const Vector3D& b = triV[t][1];
                const Vector3D& c = triV[t][2];
                Vector3D cs, ct;
                const float distSq = SegmentTriangleClosest(pBotLoc, pTopLoc, a, b, c, cs, ct);
                if (distSq > rSq) continue;
                Vector3D normalLoc;
                Vector3D contactPointLoc;
                float    contactDepth;
                if (distSq > 1e-12f) {
                    const float dist = std::sqrt(distSq);
                    normalLoc       = (cs - ct) * (1.0f / dist);
                    contactDepth    = r - dist;
                    contactPointLoc = ct;
                } else {
                    const Vector3D fn = Cross(b - a, c - a);
                    const float    fm = std::sqrt(Dot(fn, fn));
                    Vector3D nl = (fm > 1e-12f) ? fn * (1.0f / fm) : Vector3D(0,1,0);
                    const float dB = Dot(nl, pBotLoc - a);
                    const float dT = Dot(nl, pTopLoc - a);
                    const float dMin = std::min(dB, dT);
                    const float dMax = std::max(dB, dT);
                    const float pushPlus  = r - dMin;
                    const float pushMinus = dMax + r;
                    if (pushPlus <= pushMinus) {
                        contactDepth        = pushPlus;
                        const Vector3D deep = (dB < dT) ? pBotLoc : pTopLoc;
                        contactPointLoc     = deep - nl * dMin;
                        normalLoc           = nl;
                    } else {
                        contactDepth        = pushMinus;
                        const Vector3D deep = (dB > dT) ? pBotLoc : pTopLoc;
                        contactPointLoc     = deep - nl * dMax;
                        normalLoc           = nl * -1.0f;
                    }
                }
                Contact cc;
                cc.normal    = pb.orientation.RotateVector(normalLoc);
                cc.depth     = contactDepth;
                cc.point     = pb.position + pb.orientation.RotateVector(contactPointLoc);
                cc.featureId = (cellLin << 1) | static_cast<std::uint64_t>(t);
                out.AddContact(cc);
                anyHit = true;
            }
        }
    }
    return anyHit && out.count > 0;
}

// -- Box vs Heightfield -------------------------------------------------------
// Mirrors BoxTriangleMesh per-tri SAT, but collects contacts into a candidate
// vector then reduces via deepest + spread before populating the 4-slot
// manifold. Necessary because box-on-flat-heightfield can generate many
// equal-depth contacts; streaming AddContact would bias by traversal order.
bool BoxHeightfield(const BoxShape& box, const BodyPose& pa,
                    const HeightfieldShape& hf, const BodyPose& pb,
                    ContactManifold& out) {
    const HeightfieldData* data = hf.Data();
    if (!data || data->Empty()) return false;

    const Vector3D he       = box.halfExtents;
    const Vector3D boxC_loc = pb.orientation.UnrotateVector(pa.position - pb.position);
    const Vector3D u0_loc = pb.orientation.UnrotateVector(pa.orientation.RotateVector(Vector3D(1,0,0)));
    const Vector3D u1_loc = pb.orientation.UnrotateVector(pa.orientation.RotateVector(Vector3D(0,1,0)));
    const Vector3D u2_loc = pb.orientation.UnrotateVector(pa.orientation.RotateVector(Vector3D(0,0,1)));

    const float ex = std::fabs(u0_loc.X)*he.X + std::fabs(u1_loc.X)*he.Y + std::fabs(u2_loc.X)*he.Z;
    const float ez = std::fabs(u0_loc.Z)*he.X + std::fabs(u1_loc.Z)*he.Y + std::fabs(u2_loc.Z)*he.Z;

    std::uint32_t i0, i1, j0, j1;
    if (!CellRange(boxC_loc.X - ex, boxC_loc.X + ex, data->widthCells, data->cellSize, i0, i1)) return false;
    if (!CellRange(boxC_loc.Z - ez, boxC_loc.Z + ez, data->depthCells, data->cellSize, j0, j1)) return false;

    Vector3D boxVerts[8];
    for (int k = 0; k < 8; ++k) {
        const float sx = (k & 1) ? he.X : -he.X;
        const float sy = (k & 2) ? he.Y : -he.Y;
        const float sz = (k & 4) ? he.Z : -he.Z;
        boxVerts[k] = boxC_loc + u0_loc * sx + u1_loc * sy + u2_loc * sz;
    }
    static const int faceVtx[6][4] = {
        {0,2,6,4}, {1,5,7,3}, {0,4,5,1}, {2,3,7,6}, {0,1,3,2}, {4,6,7,5}
    };
    const Vector3D faceAxes[6] = {
        u0_loc * -1.0f, u0_loc, u1_loc * -1.0f, u1_loc, u2_loc * -1.0f, u2_loc
    };

    auto projectBoxAlongAxis = [&](const Vector3D& axis, float& mn, float& mx) {
        const float c   = Dot(boxC_loc, axis);
        const float ex2 = std::fabs(Dot(u0_loc, axis))*he.X
                        + std::fabs(Dot(u1_loc, axis))*he.Y
                        + std::fabs(Dot(u2_loc, axis))*he.Z;
        mn = c - ex2;
        mx = c + ex2;
    };

    std::vector<Contact> cands;
    cands.reserve(16);

    for (std::uint32_t jj = j0; jj <= j1; ++jj) {
        for (std::uint32_t ii = i0; ii <= i1; ++ii) {
            Vector3D v00, v10, v11, v01;
            CellTriangles(*data, ii, jj, v00, v10, v11, v01);
            const std::uint64_t cellLin = static_cast<std::uint64_t>(jj) * data->widthCells + ii;
            const Vector3D triV[2][3] = {
                { v00, v11, v10 },
                { v00, v01, v11 },
            };
            for (int t = 0; t < 2; ++t) {
                const Vector3D& v0 = triV[t][0];
                const Vector3D& v1 = triV[t][1];
                const Vector3D& v2 = triV[t][2];
                const Vector3D e0 = v1 - v0;
                const Vector3D e1 = v2 - v1;
                const Vector3D e2 = v0 - v2;
                const Vector3D triN_unnorm = Cross(e0, v2 - v0);
                const float    triN_magSq  = Dot(triN_unnorm, triN_unnorm);
                if (triN_magSq < 1e-20f) continue;
                const Vector3D triN = triN_unnorm * (1.0f / std::sqrt(triN_magSq));

                Vector3D bestAxis(0,0,0);
                float    bestOverlap = std::numeric_limits<float>::max();
                bool     anyAxis = false;
                bool     separating = false;

                auto tryAxis = [&](const Vector3D& axisRaw) {
                    if (separating) return;
                    const float magSq = Dot(axisRaw, axisRaw);
                    if (magSq < 1e-12f) return;
                    const Vector3D axis = axisRaw * (1.0f / std::sqrt(magSq));
                    float boxMn, boxMx;
                    projectBoxAlongAxis(axis, boxMn, boxMx);
                    const float t0 = Dot(v0, axis);
                    const float t1 = Dot(v1, axis);
                    const float t2 = Dot(v2, axis);
                    const float triMn = std::min(t0, std::min(t1, t2));
                    const float triMx = std::max(t0, std::max(t1, t2));
                    if (boxMx < triMn || triMx < boxMn) { separating = true; return; }
                    const float overlap = std::min(boxMx - triMn, triMx - boxMn);
                    if (overlap < bestOverlap) {
                        bestOverlap = overlap;
                        bestAxis    = axis;
                        anyAxis     = true;
                    }
                };

                tryAxis(u0_loc); tryAxis(u1_loc); tryAxis(u2_loc); tryAxis(triN);
                tryAxis(Cross(u0_loc, e0)); tryAxis(Cross(u0_loc, e1)); tryAxis(Cross(u0_loc, e2));
                tryAxis(Cross(u1_loc, e0)); tryAxis(Cross(u1_loc, e1)); tryAxis(Cross(u1_loc, e2));
                tryAxis(Cross(u2_loc, e0)); tryAxis(Cross(u2_loc, e1)); tryAxis(Cross(u2_loc, e2));
                if (separating || !anyAxis) continue;
                // Skip touching-only candidates (overlap ≈ 0): the box just kisses
                // the cell-tri at a shared edge/vertex with another cell. Those
                // produce zero-depth contacts with arbitrary edge-axis normals.
                if (bestOverlap < 1e-5f) continue;

                Vector3D nLoc = bestAxis;
                const Vector3D triCent = (v0 + v1 + v2) * (1.0f / 3.0f);
                if (Dot(nLoc, boxC_loc - triCent) < 0.0f) nLoc = nLoc * -1.0f;

                const Vector3D normalWorld = pb.orientation.RotateVector(nLoc);
                const float faceAlign = std::fabs(Dot(nLoc, triN));

                const std::uint64_t triFeatureBase = ((cellLin << 1) | static_cast<std::uint64_t>(t)) << 4;
                bool emitted = false;

                if (faceAlign > 0.95f) {
                    int bestFace = 0;
                    float bestDot = -std::numeric_limits<float>::max();
                    for (int f = 0; f < 6; ++f) {
                        const float d = Dot(faceAxes[f], nLoc * -1.0f);
                        if (d > bestDot) { bestDot = d; bestFace = f; }
                    }
                    for (int k = 0; k < 4; ++k) {
                        const Vector3D& V = boxVerts[faceVtx[bestFace][k]];
                        const float    s  = Dot(triN, V - v0);
                        const Vector3D Vp = V - triN * s;
                        if (!PointInTriangle(Vp, v0, v1, v2)) continue;
                        const float depth = -Dot(nLoc, V - v0);
                        if (depth <= 0.0f) continue;
                        Contact c;
                        c.normal    = normalWorld;
                        c.depth     = depth;
                        c.point     = pb.position + pb.orientation.RotateVector(Vp);
                        c.featureId = triFeatureBase | static_cast<std::uint64_t>(k);
                        cands.push_back(c);
                        emitted = true;
                    }
                    int axisIdx = bestFace / 2;
                    Vector3D inFaceA, inFaceB;
                    float heA, heB;
                    if (axisIdx == 0) { inFaceA = u1_loc; inFaceB = u2_loc; heA = he.Y; heB = he.Z; }
                    else if (axisIdx == 1) { inFaceA = u0_loc; inFaceB = u2_loc; heA = he.X; heB = he.Z; }
                    else { inFaceA = u0_loc; inFaceB = u1_loc; heA = he.X; heB = he.Y; }
                    const Vector3D triVerts[3] = { v0, v1, v2 };
                    for (int k = 0; k < 3; ++k) {
                        const Vector3D& V  = triVerts[k];
                        const Vector3D rel = V - boxC_loc;
                        const float    a   = Dot(rel, inFaceA);
                        const float    b   = Dot(rel, inFaceB);
                        if (std::fabs(a) > heA + 1e-5f || std::fabs(b) > heB + 1e-5f) continue;
                        const float boxExtAlongN = std::fabs(Dot(u0_loc, nLoc))*he.X
                                                 + std::fabs(Dot(u1_loc, nLoc))*he.Y
                                                 + std::fabs(Dot(u2_loc, nLoc))*he.Z;
                        // V is the tri vertex; nLoc points from the tri toward the box.
                        // The box extent along nLoc is centered on boxC_loc with halfwidth
                        // boxExtAlongN. V is "below" the box bottom face (along -nLoc) by
                        // boxExtAlongN + Dot(nLoc, V - boxC_loc) when this is > 0; that's
                        // the penetration of V past the bottom face into the box.
                        const float depth = boxExtAlongN + Dot(nLoc, V - boxC_loc);
                        if (depth <= 0.0f) continue;
                        Contact c;
                        c.normal    = normalWorld;
                        c.depth     = depth;
                        c.point     = pb.position + pb.orientation.RotateVector(V);
                        c.featureId = triFeatureBase | (4u + static_cast<std::uint64_t>(k));
                        cands.push_back(c);
                        emitted = true;
                    }
                }

                if (!emitted) {
                    Vector3D deepestV = boxVerts[0];
                    float    minDot   = Dot(nLoc, boxVerts[0]);
                    for (int k = 1; k < 8; ++k) {
                        const float d = Dot(nLoc, boxVerts[k]);
                        if (d < minDot) { minDot = d; deepestV = boxVerts[k]; }
                    }
                    const Vector3D triClosest = ClosestPointOnTriangle(deepestV, v0, v1, v2);
                    const Vector3D pointLoc   = (deepestV + triClosest) * 0.5f;
                    Contact c;
                    c.normal    = normalWorld;
                    c.depth     = bestOverlap;
                    c.point     = pb.position + pb.orientation.RotateVector(pointLoc);
                    c.featureId = triFeatureBase;
                    cands.push_back(c);
                }
            }
        }
    }

    ReduceCandidatesToManifold(cands, out);
    return out.count > 0;
}

} // namespace

bool ManifoldGenerator::Generate(ColliderProxy* a, const BodyPose& poseA,
                                 ColliderProxy* b, const BodyPose& poseB,
                                 ContactManifold& out) {
    if (!a || !b || !a->shape || !b->shape) return false;

    // Canonicalize so a->proxyId < b->proxyId. Swap callers' arguments
    // (and poses) accordingly so 'normal points from B toward A' stays well-defined
    // relative to whichever pair the cache will key on.
    ColliderProxy* A = a;
    ColliderProxy* B = b;
    const BodyPose* PA = &poseA;
    const BodyPose* PB = &poseB;
    if (B->proxyId < A->proxyId) {
        std::swap(A, B);
        std::swap(PA, PB);
    }

    out.Clear();
    out.a = A;
    out.b = B;

    const ShapeType ta = A->shape->Type();
    const ShapeType tb = B->shape->Type();
    const BodyPose worldA = WorldPoseOf(*A, *PA);
    const BodyPose worldB = WorldPoseOf(*B, *PB);

    // Sphere-Sphere
    if (ta == ShapeType::Sphere && tb == ShapeType::Sphere) {
        return SphereSphere(static_cast<const SphereShape&>(*A->shape), worldA,
                            static_cast<const SphereShape&>(*B->shape), worldB, out);
    }
    // Sphere-Plane
    if (ta == ShapeType::Sphere && tb == ShapeType::Plane) {
        return SpherePlane(static_cast<const SphereShape&>(*A->shape), worldA,
                           static_cast<const PlaneShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::Plane && tb == ShapeType::Sphere) {
        // Re-canonicalize so sphere is A: build manifold with swapped sides, flip normal.
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = SpherePlane(static_cast<const SphereShape&>(*B->shape), worldB,
                                     static_cast<const PlaneShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }
    // Sphere-Box
    if (ta == ShapeType::Sphere && tb == ShapeType::Box) {
        return SphereBox(static_cast<const SphereShape&>(*A->shape), worldA,
                         static_cast<const BoxShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::Box && tb == ShapeType::Sphere) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = SphereBox(static_cast<const SphereShape&>(*B->shape), worldB,
                                   static_cast<const BoxShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }
    // Convex/Plane (any non-sphere convex against a plane).
    if (tb == ShapeType::Plane && A->shape->IsConvex()) {
        return ConvexPlane(*A->shape, worldA,
                           static_cast<const PlaneShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::Plane && B->shape->IsConvex()) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = ConvexPlane(*B->shape, worldB,
                                     static_cast<const PlaneShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // Box-Box: SAT + face clipping (multi-contact).
    if (ta == ShapeType::Box && tb == ShapeType::Box) {
        return BoxBoxContact(static_cast<const BoxShape&>(*A->shape), worldA,
                             static_cast<const BoxShape&>(*B->shape), worldB, out);
    }

    // Sphere vs TriangleMesh.
    if (ta == ShapeType::Sphere && tb == ShapeType::TriangleMesh) {
        return SphereTriangleMesh(static_cast<const SphereShape&>(*A->shape), worldA,
                                  static_cast<const TriangleMeshShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::TriangleMesh && tb == ShapeType::Sphere) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = SphereTriangleMesh(static_cast<const SphereShape&>(*B->shape), worldB,
                                            static_cast<const TriangleMeshShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // Capsule vs TriangleMesh.
    if (ta == ShapeType::Capsule && tb == ShapeType::TriangleMesh) {
        return CapsuleTriangleMesh(static_cast<const CapsuleShape&>(*A->shape), worldA,
                                   static_cast<const TriangleMeshShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::TriangleMesh && tb == ShapeType::Capsule) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = CapsuleTriangleMesh(static_cast<const CapsuleShape&>(*B->shape), worldB,
                                             static_cast<const TriangleMeshShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // Box vs TriangleMesh.
    if (ta == ShapeType::Box && tb == ShapeType::TriangleMesh) {
        return BoxTriangleMesh(static_cast<const BoxShape&>(*A->shape), worldA,
                               static_cast<const TriangleMeshShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::TriangleMesh && tb == ShapeType::Box) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = BoxTriangleMesh(static_cast<const BoxShape&>(*B->shape), worldB,
                                         static_cast<const TriangleMeshShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // Sphere vs Heightfield.
    if (ta == ShapeType::Sphere && tb == ShapeType::HeightField) {
        return SphereHeightfield(static_cast<const SphereShape&>(*A->shape), worldA,
                                 static_cast<const HeightfieldShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::HeightField && tb == ShapeType::Sphere) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = SphereHeightfield(static_cast<const SphereShape&>(*B->shape), worldB,
                                           static_cast<const HeightfieldShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // Capsule vs Heightfield.
    if (ta == ShapeType::Capsule && tb == ShapeType::HeightField) {
        return CapsuleHeightfield(static_cast<const CapsuleShape&>(*A->shape), worldA,
                                  static_cast<const HeightfieldShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::HeightField && tb == ShapeType::Capsule) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = CapsuleHeightfield(static_cast<const CapsuleShape&>(*B->shape), worldB,
                                            static_cast<const HeightfieldShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // Box vs Heightfield.
    if (ta == ShapeType::Box && tb == ShapeType::HeightField) {
        return BoxHeightfield(static_cast<const BoxShape&>(*A->shape), worldA,
                              static_cast<const HeightfieldShape&>(*B->shape), worldB, out);
    }
    if (ta == ShapeType::HeightField && tb == ShapeType::Box) {
        ContactManifold tmp;
        tmp.a = B; tmp.b = A;
        const bool hit = BoxHeightfield(static_cast<const BoxShape&>(*B->shape), worldB,
                                        static_cast<const HeightfieldShape&>(*A->shape), worldA, tmp);
        if (!hit) return false;
        for (std::uint8_t i = 0; i < tmp.count; ++i) {
            Contact c = tmp.contacts[i];
            c.normal = c.normal * -1.0f;
            out.AddContact(c);
        }
        return out.count > 0;
    }

    // General convex/convex via GJK + EPA.
    if (A->shape->IsConvex() && B->shape->IsConvex()) {
        return GjkEpaPair(*A->shape, worldA, *B->shape, worldB, out);
    }
    return false;
}

// =====================================================================
// speculative-contact closest-feature queries.
//
// These functions return:
//   - true  : a speculative contact was emitted into `out` (depth = -gap,
//             isSpeculative = true).
//   - false : either the gap exceeds `margin` (no need for speculation) or
//             the pair already overlaps (caller should have used Generate).
//
// Convention matches Generate: normal points from B toward A. The contact
// `point` is placed on B's witness location (the surface point closest to A).
// =====================================================================
namespace {

// Returns the closest point on segment [P0,P1] to query point Q.
inline Vector3D ClosestPointOnSegment(const Vector3D& Q,
                                      const Vector3D& P0, const Vector3D& P1) {
    const Vector3D ab = P1 - P0;
    const float ab2 = Dot(ab, ab);
    if (ab2 < 1e-12f) return P0;
    float t = Dot(Q - P0, ab) / ab2;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    return P0 + ab * t;
}

// Speculative sphere-sphere: gap = distance(centers) - (rA + rB).
bool SpeculativeSphereSphere(const SphereShape& sa, const BodyPose& pa,
                             const SphereShape& sb, const BodyPose& pb,
                             float margin, ContactManifold& out) {
    const Vector3D delta = pa.position - pb.position; // B -> A
    const float distSq = Dot(delta, delta);
    const float radiusSum = sa.radius + sb.radius;
    const float dist = std::sqrt(distSq);
    const float gap = dist - radiusSum;
    if (gap < 0.0f) return false; // overlap; caller should use Generate
    if (gap >= margin) return false;
    Vector3D normal = (dist > 1e-6f) ? delta * (1.0f / dist) : Vector3D(0, 1, 0);
    Contact c;
    c.normal = normal;
    c.depth = -gap;
    c.point = pb.position + normal * sb.radius; // witness on B
    c.featureId = 0;
    out.AddContact(c);
    out.isSpeculative = true;
    return out.count > 0;
}

// Speculative sphere-plane: gap = (sphereCenter·n - d) - r.
bool SpeculativeSpherePlane(const SphereShape& sa, const BodyPose& pa,
                            const PlaneShape& pl, const BodyPose& pb,
                            float margin, ContactManifold& out) {
    const Vector3D n = pl.NormalWorld(pb);
    const float d = pl.DistanceWorld(pb);
    const float signedDist = Dot(n, pa.position) - d;
    const float gap = signedDist - sa.radius;
    if (gap < 0.0f) return false;
    if (gap >= margin) return false;
    Contact c;
    c.normal = n;          // plane normal already separates B from A
    c.depth = -gap;
    c.point = pa.position - n * sa.radius; // witness on A's near side
    c.featureId = 0;
    out.AddContact(c);
    out.isSpeculative = true;
    return out.count > 0;
}

// Speculative sphere-box: closest point on the box (in box-local space) to the
// sphere center; gap = |delta| - r. Mirrors SphereBox in Generate but emits a
// negative-depth contact when gap < margin.
bool SpeculativeSphereBox(const SphereShape& sa, const BodyPose& pa,
                          const BoxShape& bx, const BodyPose& pb,
                          float margin, ContactManifold& out) {
    const Vector3D local = pb.orientation.UnrotateVector(pa.position - pb.position);
    const Vector3D he = bx.halfExtents;
    const Vector3D closestLocal(
        std::max(-he.X, std::min(local.X, he.X)),
        std::max(-he.Y, std::min(local.Y, he.Y)),
        std::max(-he.Z, std::min(local.Z, he.Z))
    );
    const Vector3D deltaLocal = local - closestLocal;
    const float distSq = Dot(deltaLocal, deltaLocal);
    if (distSq < 1e-12f) return false; // sphere center inside box -> overlap path
    const float dist = std::sqrt(distSq);
    const float gap = dist - sa.radius;
    if (gap < 0.0f) return false;
    if (gap >= margin) return false;
    const Vector3D normalLocal = deltaLocal * (1.0f / dist);
    const Vector3D normalWorld = pb.orientation.RotateVector(normalLocal); // points B->A
    const Vector3D witnessWorld = pb.position + pb.orientation.RotateVector(closestLocal);
    Contact c;
    c.normal = normalWorld;
    c.depth = -gap;
    c.point = witnessWorld;
    c.featureId = 0;
    out.AddContact(c);
    out.isSpeculative = true;
    return out.count > 0;
}

// Speculative sphere-capsule: closest point on capsule axis to sphere center,
// then it reduces to sphere-sphere with capsule's hemispherical radius.
bool SpeculativeSphereCapsule(const SphereShape& sa, const BodyPose& pa,
                              const CapsuleShape& cap, const BodyPose& pb,
                              float margin, ContactManifold& out) {
    const Vector3D axisLocal(0.0f, cap.halfHeight, 0.0f);
    const Vector3D axisWorld = pb.orientation.RotateVector(axisLocal);
    const Vector3D P0 = pb.position - axisWorld;
    const Vector3D P1 = pb.position + axisWorld;
    const Vector3D witnessSeg = ClosestPointOnSegment(pa.position, P0, P1);
    const Vector3D delta = pa.position - witnessSeg; // points capsule->sphere
    const float distSq = Dot(delta, delta);
    const float radiusSum = sa.radius + cap.radius;
    const float dist = std::sqrt(distSq);
    const float gap = dist - radiusSum;
    if (gap < 0.0f) return false;
    if (gap >= margin) return false;
    Vector3D normal = (dist > 1e-6f) ? delta * (1.0f / dist) : Vector3D(0, 1, 0);
    Contact c;
    c.normal = normal;
    c.depth = -gap;
    c.point = witnessSeg + normal * cap.radius; // witness on capsule surface
    c.featureId = 0;
    out.AddContact(c);
    out.isSpeculative = true;
    return out.count > 0;
}

} // namespace

bool ManifoldGenerator::GenerateSpeculative(ColliderProxy* a, const BodyPose& poseA,
                                            ColliderProxy* b, const BodyPose& poseB,
                                            float margin,
                                            ContactManifold& out) {
    if (!a || !b || !a->shape || !b->shape) return false;
    if (margin <= 0.0f) return false;

    ColliderProxy* A = a;
    ColliderProxy* B = b;
    const BodyPose* PA = &poseA;
    const BodyPose* PB = &poseB;
    if (B->proxyId < A->proxyId) {
        std::swap(A, B);
        std::swap(PA, PB);
    }

    out.Clear();
    out.a = A;
    out.b = B;

    const ShapeType ta = A->shape->Type();
    const ShapeType tb = B->shape->Type();
    const BodyPose worldA = WorldPoseOf(*A, *PA);
    const BodyPose worldB = WorldPoseOf(*B, *PB);

    // Sphere-Sphere
    if (ta == ShapeType::Sphere && tb == ShapeType::Sphere) {
        return SpeculativeSphereSphere(static_cast<const SphereShape&>(*A->shape), worldA,
                                       static_cast<const SphereShape&>(*B->shape), worldB,
                                       margin, out);
    }
    // Sphere-Plane (any order). Place sphere as A so normal points plane->sphere.
    if (ta == ShapeType::Sphere && tb == ShapeType::Plane) {
        return SpeculativeSpherePlane(static_cast<const SphereShape&>(*A->shape), worldA,
                                      static_cast<const PlaneShape&>(*B->shape), worldB,
                                      margin, out);
    }
    if (ta == ShapeType::Plane && tb == ShapeType::Sphere) {
        const bool hit = SpeculativeSpherePlane(static_cast<const SphereShape&>(*B->shape), worldB,
                                                static_cast<const PlaneShape&>(*A->shape), worldA,
                                                margin, out);
        if (hit) {
            for (std::uint8_t i = 0; i < out.count; ++i)
                out.contacts[i].normal = out.contacts[i].normal * -1.0f;
        }
        return hit;
    }
    // Sphere-Box (any order).
    if (ta == ShapeType::Sphere && tb == ShapeType::Box) {
        return SpeculativeSphereBox(static_cast<const SphereShape&>(*A->shape), worldA,
                                    static_cast<const BoxShape&>(*B->shape), worldB,
                                    margin, out);
    }
    if (ta == ShapeType::Box && tb == ShapeType::Sphere) {
        const bool hit = SpeculativeSphereBox(static_cast<const SphereShape&>(*B->shape), worldB,
                                              static_cast<const BoxShape&>(*A->shape), worldA,
                                              margin, out);
        if (hit) {
            for (std::uint8_t i = 0; i < out.count; ++i)
                out.contacts[i].normal = out.contacts[i].normal * -1.0f;
        }
        return hit;
    }
    // Sphere-Capsule (any order).
    if (ta == ShapeType::Sphere && tb == ShapeType::Capsule) {
        return SpeculativeSphereCapsule(static_cast<const SphereShape&>(*A->shape), worldA,
                                        static_cast<const CapsuleShape&>(*B->shape), worldB,
                                        margin, out);
    }
    if (ta == ShapeType::Capsule && tb == ShapeType::Sphere) {
        const bool hit = SpeculativeSphereCapsule(static_cast<const SphereShape&>(*B->shape), worldB,
                                                  static_cast<const CapsuleShape&>(*A->shape), worldA,
                                                  margin, out);
        if (hit) {
            for (std::uint8_t i = 0; i < out.count; ++i)
                out.contacts[i].normal = out.contacts[i].normal * -1.0f;
        }
        return hit;
    }
    // Other pair types: speculative path not yet implemented (tunneling possible).
    return false;
}

} // namespace koilo
