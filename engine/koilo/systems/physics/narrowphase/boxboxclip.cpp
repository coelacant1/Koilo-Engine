// SPDX-License-Identifier: GPL-3.0-or-later
#include "boxboxclip.hpp"
#include <koilo/core/geometry/3d/obb.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace koilo {

namespace {

constexpr float kFaceBias = 1.0e-3f;
constexpr float kEps      = 1.0e-6f;

inline float Dot(const Vector3D& a, const Vector3D& b) {
    return a.X*b.X + a.Y*b.Y + a.Z*b.Z;
}

OBB MakeOBB(const BoxShape& box, const BodyPose& pose) {
    return OBB(pose.position, box.halfExtents, pose.orientation);
}

struct ClippedVertex {
    Vector3D pos;
    int      srcIncEdge = -1;   ///< Incident-edge id (0..3) the point came from; -1 = original incident vertex.
};

/**
 * Returns the four world-space corners of a box face.
 * @param obb The box.
 * @param axisIdx Face axis (0..2).
 * @param sign +1 = +axis face, -1 = -axis face.
 * @param out Out array of 4 vertices in CCW order viewed from outside.
 */
void GetFaceVertices(const OBB& obb, int axisIdx, float sign, Vector3D out[4]) {
    const Vector3D ax[3] = { obb.AxisX(), obb.AxisY(), obb.AxisZ() };
    const float    he[3] = { obb.halfExtents.X, obb.halfExtents.Y, obb.halfExtents.Z };
    const int u = (axisIdx + 1) % 3;
    const int v = (axisIdx + 2) % 3;
    const Vector3D faceCenter = obb.center + ax[axisIdx] * (sign * he[axisIdx]);
    const Vector3D du = ax[u] * he[u];
    const Vector3D dv = ax[v] * he[v];
    out[0] = faceCenter - du - dv;
    out[1] = faceCenter + du - dv;
    out[2] = faceCenter + du + dv;
    out[3] = faceCenter - du + dv;
}

/**
 * Clips a polygon against a single half-space defined by `(planeNormal · p) <= planeOffset`
 * (i.e. interior is where signed distance `planeOffset - planeNormal·p >= 0`).
 * Sutherland-Hodgman, with provenance tracking via srcIncEdge.
 */
void ClipAgainstPlane(const std::vector<ClippedVertex>& in,
                      const Vector3D& planeNormal, float planeOffset,
                      int planeId,
                      std::vector<ClippedVertex>& out) {
    out.clear();
    if (in.empty()) return;
    const std::size_t n = in.size();
    for (std::size_t i = 0; i < n; ++i) {
        const ClippedVertex& cur = in[i];
        const ClippedVertex& nxt = in[(i + 1) % n];
        const float distCur = planeOffset - Dot(planeNormal, cur.pos);
        const float distNxt = planeOffset - Dot(planeNormal, nxt.pos);
        const bool curIn = distCur >= 0.0f;
        const bool nxtIn = distNxt >= 0.0f;
        if (curIn) out.push_back(cur);
        if (curIn != nxtIn) {
            const float t = distCur / (distCur - distNxt);
            ClippedVertex c;
            c.pos = cur.pos + (nxt.pos - cur.pos) * t;
            // The edge that just crossed the plane carries the plane id as its
            // source-edge tag. Combined with the original incident face id this
            // yields a stable feature id across small movements.
            c.srcIncEdge = planeId;
            out.push_back(c);
        }
    }
}

/**
 * Closest points between two finite segments (line-segment vs line-segment),
 * used for the edge-edge contact path. Returns the midpoint of the two
 * closest points and the distance between them.
 */
void ClosestSegmentPoints(const Vector3D& p1, const Vector3D& q1,
                          const Vector3D& p2, const Vector3D& q2,
                          Vector3D& outClosest1, Vector3D& outClosest2) {
    const Vector3D d1 = q1 - p1;
    const Vector3D d2 = q2 - p2;
    const Vector3D r  = p1 - p2;
    const float a = Dot(d1, d1);
    const float e = Dot(d2, d2);
    const float f = Dot(d2, r);
    float s = 0.0f, t = 0.0f;
    if (a <= kEps && e <= kEps) {
        outClosest1 = p1; outClosest2 = p2; return;
    }
    if (a <= kEps) {
        t = std::max(0.0f, std::min(1.0f, f / e));
    } else {
        const float c = Dot(d1, r);
        if (e <= kEps) {
            s = std::max(0.0f, std::min(1.0f, -c / a));
        } else {
            const float b = Dot(d1, d2);
            const float denom = a*e - b*b;
            if (denom != 0.0f) {
                s = std::max(0.0f, std::min(1.0f, (b*f - c*e) / denom));
            }
            t = (b*s + f) / e;
            if (t < 0.0f) { t = 0.0f; s = std::max(0.0f, std::min(1.0f, -c / a)); }
            else if (t > 1.0f) { t = 1.0f; s = std::max(0.0f, std::min(1.0f, (b - c) / a)); }
        }
    }
    outClosest1 = p1 + d1 * s;
    outClosest2 = p2 + d2 * t;
}

bool BoxBoxFaceContact(const OBB& obbA, const OBB& obbB,
                       const OBB::SeparationInfo& info,
                       ContactManifold& out) {
    const bool refIsA  = (info.axisIndex < 3);
    const int  refAxis = refIsA ? info.axisIndex : (info.axisIndex - 3);
    const OBB& refObb  = refIsA ? obbA : obbB;
    const OBB& incObb  = refIsA ? obbB : obbA;

    // Reference outward normal: refObb's face that points toward the OTHER box.
    const Vector3D refAxes[3] = { refObb.AxisX(), refObb.AxisY(), refObb.AxisZ() };
    const float    refHE[3]   = { refObb.halfExtents.X, refObb.halfExtents.Y, refObb.halfExtents.Z };
    const float    refSign    = (Dot(refAxes[refAxis], incObb.center - refObb.center) >= 0.0f) ? 1.0f : -1.0f;
    const Vector3D refOutward = refAxes[refAxis] * refSign;
    const Vector3D refCenter  = refObb.center + refOutward * refHE[refAxis];
    // Contact normal points from B to A.
    const Vector3D contactNormal = refIsA ? (refOutward * -1.0f) : refOutward;

    // Incident face: face of incObb whose outward normal is most antiparallel to refOutward.
    const Vector3D incAxes[3] = { incObb.AxisX(), incObb.AxisY(), incObb.AxisZ() };
    int   incAxis = 0;
    float incSign = 1.0f;
    {
        float worst = 1e30f;
        for (int i = 0; i < 3; ++i) {
            for (int s = -1; s <= 1; s += 2) {
                const float dp = Dot(incAxes[i] * static_cast<float>(s), refOutward);
                if (dp < worst) { worst = dp; incAxis = i; incSign = static_cast<float>(s); }
            }
        }
    }

    Vector3D incFace[4];
    GetFaceVertices(incObb, incAxis, incSign, incFace);

    // Build initial polygon (4 incident-face vertices, srcIncEdge = -1 for originals).
    std::vector<ClippedVertex> polyA, polyB;
    polyA.reserve(8); polyB.reserve(8);
    for (int i = 0; i < 4; ++i) polyA.push_back({ incFace[i], -1 });

    // Reference face in-plane axes (the two non-ref axes of refObb).
    const int u = (refAxis + 1) % 3;
    const int v = (refAxis + 2) % 3;
    const Vector3D tu = refAxes[u];
    const Vector3D tv = refAxes[v];
    const float    hu = refHE[u];
    const float    hv = refHE[v];
    // Side planes: (n · p) <= offset. Interior signed distance = offset - n·p.
    const float refCu = Dot(tu, refCenter);
    const float refCv = Dot(tv, refCenter);
    ClipAgainstPlane(polyA, tu,           refCu + hu, 0, polyB);
    ClipAgainstPlane(polyB, tu * -1.0f, -(refCu - hu), 1, polyA);
    ClipAgainstPlane(polyA, tv,           refCv + hv, 2, polyB);
    ClipAgainstPlane(polyB, tv * -1.0f, -(refCv - hv), 3, polyA);

    if (polyA.empty()) return false;

    // Compute depth for each surviving vertex (penetration past ref face).
    // depth = -dot(p - refCenter, refOutward). Positive = penetrating.
    struct Candidate { Vector3D pos; float depth; int srcIncEdge; };
    std::vector<Candidate> cands;
    cands.reserve(polyA.size());
    for (const auto& cv : polyA) {
        const float depth = -Dot(cv.pos - refCenter, refOutward);
        if (depth > 0.0f) cands.push_back({ cv.pos, depth, cv.srcIncEdge });
    }
    if (cands.empty()) return false;

    // Sort deepest first so manifold's deepest-replacement keeps the best 4.
    std::sort(cands.begin(), cands.end(),
        [](const Candidate& x, const Candidate& y){ return x.depth > y.depth; });

    const std::uint32_t refFaceId  = static_cast<std::uint32_t>(refAxis * 2 + (refSign > 0 ? 0 : 1));
    const std::uint32_t incFaceId  = static_cast<std::uint32_t>(incAxis * 2 + (incSign > 0 ? 0 : 1));
    const std::uint32_t refIsAFlag = refIsA ? 1u : 0u;

    int clipIdx = 0;
    for (const auto& c : cands) {
        Contact contact;
        contact.normal = contactNormal;
        contact.depth  = c.depth;
        contact.point  = c.pos;
        contact.featureId =
            (static_cast<std::uint32_t>(clipIdx) & 0xFFFFu)
            | (incFaceId  << 16)
            | (refFaceId  << 22)
            | (refIsAFlag << 28)
            | (0u         << 29);   // axisType = 0 (face contact)
        // Mix in the source-edge tag so an interior point and an edge-clipped
        // point produce distinct ids for the same incident face id.
        if (c.srcIncEdge >= 0) {
            contact.featureId ^= (static_cast<std::uint32_t>(c.srcIncEdge + 1) << 12);
        }
        out.AddContact(contact);
        ++clipIdx;
    }
    return out.count > 0;
}

bool BoxBoxEdgeContact(const OBB& obbA, const OBB& obbB,
                       const OBB::SeparationInfo& info,
                       ContactManifold& out) {
    const int idx = info.axisIndex - 6;
    const int i   = idx / 3;     ///< A axis (edge direction).
    const int j   = idx % 3;     ///< B axis (edge direction).
    const Vector3D aAx[3] = { obbA.AxisX(), obbA.AxisY(), obbA.AxisZ() };
    const Vector3D bAx[3] = { obbB.AxisX(), obbB.AxisY(), obbB.AxisZ() };
    const float    aHE[3] = { obbA.halfExtents.X, obbA.halfExtents.Y, obbA.halfExtents.Z };
    const float    bHE[3] = { obbB.halfExtents.X, obbB.halfExtents.Y, obbB.halfExtents.Z };

    // Pick the edge on each box closest to the other box's center.
    const Vector3D delta = obbB.center - obbA.center;
    const int aU = (i + 1) % 3, aV = (i + 2) % 3;
    const int bU = (j + 1) % 3, bV = (j + 2) % 3;
    const float aSignU = Dot(delta,        aAx[aU]) >= 0 ? 1.0f : -1.0f;
    const float aSignV = Dot(delta,        aAx[aV]) >= 0 ? 1.0f : -1.0f;
    const float bSignU = Dot(delta * -1.0f, bAx[bU]) >= 0 ? 1.0f : -1.0f;
    const float bSignV = Dot(delta * -1.0f, bAx[bV]) >= 0 ? 1.0f : -1.0f;

    const Vector3D aEdgeMid = obbA.center + aAx[aU] * (aSignU * aHE[aU]) + aAx[aV] * (aSignV * aHE[aV]);
    const Vector3D bEdgeMid = obbB.center + bAx[bU] * (bSignU * bHE[bU]) + bAx[bV] * (bSignV * bHE[bV]);
    const Vector3D aP = aEdgeMid - aAx[i] * aHE[i];
    const Vector3D aQ = aEdgeMid + aAx[i] * aHE[i];
    const Vector3D bP = bEdgeMid - bAx[j] * bHE[j];
    const Vector3D bQ = bEdgeMid + bAx[j] * bHE[j];

    Vector3D ca, cb;
    ClosestSegmentPoints(aP, aQ, bP, bQ, ca, cb);

    // Contact normal: B -> A using info.axis (axis points A -> B).
    const Vector3D contactNormal = info.axis * -1.0f;
    Contact contact;
    contact.normal = contactNormal;
    contact.depth  = info.penetration;
    contact.point  = (ca + cb) * 0.5f;
    contact.featureId =
        ((static_cast<std::uint32_t>(i) & 0x3u))
        | ((static_cast<std::uint32_t>(j) & 0x3u) << 2)
        | (2u << 29);   // axisType = 2 (edge-edge)
    out.AddContact(contact);
    return out.count > 0;
}

} // namespace

bool BoxBoxContact(const BoxShape& a, const BodyPose& poseA,
                   const BoxShape& b, const BodyPose& poseB,
                   ContactManifold& out) {
    const OBB obbA = MakeOBB(a, poseA);
    const OBB obbB = MakeOBB(b, poseB);
    const OBB::SeparationInfo info = obbA.ComputeSeparation(obbB, kFaceBias);
    if (!info.intersect) return false;
    if (info.axisIndex < 6) {
        return BoxBoxFaceContact(obbA, obbB, info, out);
    }
    return BoxBoxEdgeContact(obbA, obbB, info, out);
}

} // namespace koilo
