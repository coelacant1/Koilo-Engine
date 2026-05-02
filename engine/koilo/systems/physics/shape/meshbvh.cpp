// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/physics/shape/meshbvh.hpp>

#include <algorithm>
#include <cmath>

namespace koilo {

namespace {

inline AABB TriangleAABB(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2) {
    AABB box(v0, v0);
    box.Encapsulate(v1);
    box.Encapsulate(v2);
    return box;
}

inline Vector3D TriangleCentroid(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2) {
    return Vector3D((v0.X + v1.X + v2.X) / 3.0f,
                    (v0.Y + v1.Y + v2.Y) / 3.0f,
                    (v0.Z + v1.Z + v2.Z) / 3.0f);
}

inline bool IsTriangleDegenerate(const Vector3D& v0, const Vector3D& v1, const Vector3D& v2,
                                 float areaEps = 1e-12f) {
    const Vector3D e1(v1.X - v0.X, v1.Y - v0.Y, v1.Z - v0.Z);
    const Vector3D e2(v2.X - v0.X, v2.Y - v0.Y, v2.Z - v0.Z);
    const Vector3D cr(e1.Y * e2.Z - e1.Z * e2.Y,
                      e1.Z * e2.X - e1.X * e2.Z,
                      e1.X * e2.Y - e1.Y * e2.X);
    return (cr.X * cr.X + cr.Y * cr.Y + cr.Z * cr.Z) < areaEps;
}

} // namespace

void TriangleMeshBVH::Build(const std::vector<Vector3D>& vertices,
                    const std::vector<std::uint32_t>& indices,
                    std::uint32_t leafSize) {
    nodes_.clear();
    triRemap_.clear();
    if (vertices.empty() || indices.size() < 3) return;
    if (leafSize == 0) leafSize = 1;

    const std::uint32_t triCount = static_cast<std::uint32_t>(indices.size() / 3);

    std::vector<AABB>     triBounds;
    std::vector<Vector3D> triCentroids;
    triBounds.reserve(triCount);
    triCentroids.reserve(triCount);
    triRemap_.reserve(triCount);

    const std::uint32_t vCount = static_cast<std::uint32_t>(vertices.size());
    for (std::uint32_t t = 0; t < triCount; ++t) {
        const std::uint32_t i0 = indices[3*t + 0];
        const std::uint32_t i1 = indices[3*t + 1];
        const std::uint32_t i2 = indices[3*t + 2];
        if (i0 >= vCount || i1 >= vCount || i2 >= vCount) continue;
        const Vector3D& v0 = vertices[i0];
        const Vector3D& v1 = vertices[i1];
        const Vector3D& v2 = vertices[i2];
        if (IsTriangleDegenerate(v0, v1, v2)) continue;

        triBounds.push_back(TriangleAABB(v0, v1, v2));
        triCentroids.push_back(TriangleCentroid(v0, v1, v2));
        triRemap_.push_back(t);
    }

    if (triRemap_.empty()) return;

    nodes_.reserve(triRemap_.size() * 2);
    BuildRecursive(triBounds, triCentroids, 0,
                   static_cast<std::uint32_t>(triRemap_.size()),
                   leafSize, 0);
}

std::int32_t TriangleMeshBVH::BuildRecursive(std::vector<AABB>& triBounds,
                                     std::vector<Vector3D>& triCentroids,
                                     std::uint32_t first, std::uint32_t count,
                                     std::uint32_t leafSize, int depth) {
    Node node;
    // Bounds = union of all included triangle AABBs.
    node.bounds = triBounds[first];
    Vector3D centroidMin = triCentroids[first];
    Vector3D centroidMax = triCentroids[first];
    for (std::uint32_t i = 1; i < count; ++i) {
        node.bounds = node.bounds.Union(triBounds[first + i]);
        const Vector3D& c = triCentroids[first + i];
        if (c.X < centroidMin.X) centroidMin.X = c.X;
        if (c.Y < centroidMin.Y) centroidMin.Y = c.Y;
        if (c.Z < centroidMin.Z) centroidMin.Z = c.Z;
        if (c.X > centroidMax.X) centroidMax.X = c.X;
        if (c.Y > centroidMax.Y) centroidMax.Y = c.Y;
        if (c.Z > centroidMax.Z) centroidMax.Z = c.Z;
    }

    if (count <= leafSize || depth >= 32) {
        node.firstTri = first;
        node.triCount = count;
        nodes_.push_back(node);
        return static_cast<std::int32_t>(nodes_.size() - 1);
    }

    const float ex = centroidMax.X - centroidMin.X;
    const float ey = centroidMax.Y - centroidMin.Y;
    const float ez = centroidMax.Z - centroidMin.Z;
    const int axis = (ex > ey && ex > ez) ? 0 : (ey > ez ? 1 : 2);

    const std::uint32_t mid = first + count / 2;

    auto axisVal = [axis](const Vector3D& v) -> float {
        return axis == 0 ? v.X : (axis == 1 ? v.Y : v.Z);
    };

    // Index permutation that we apply jointly to triBounds, triCentroids, triRemap_.
    std::vector<std::uint32_t> order(count);
    for (std::uint32_t i = 0; i < count; ++i) order[i] = first + i;

    std::nth_element(order.begin(),
                     order.begin() + count / 2,
                     order.end(),
                     [&](std::uint32_t a, std::uint32_t b) {
                         const float va = axisVal(triCentroids[a]);
                         const float vb = axisVal(triCentroids[b]);
                         if (va != vb) return va < vb;
                         return triRemap_[a] < triRemap_[b]; // deterministic tie-break
                     });

    // Apply permutation in-place.
    std::vector<AABB>          tmpB(count);
    std::vector<Vector3D>      tmpC(count);
    std::vector<std::uint32_t> tmpR(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        tmpB[i] = triBounds[order[i]];
        tmpC[i] = triCentroids[order[i]];
        tmpR[i] = triRemap_[order[i]];
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        triBounds[first + i]    = tmpB[i];
        triCentroids[first + i] = tmpC[i];
        triRemap_[first + i]    = tmpR[i];
    }

    // Reserve our slot before recursing so child indices are stable.
    const std::int32_t myIdx = static_cast<std::int32_t>(nodes_.size());
    nodes_.push_back(node); // placeholder

    const std::int32_t leftIdx  = BuildRecursive(triBounds, triCentroids,
                                                 first, mid - first,
                                                 leafSize, depth + 1);
    const std::int32_t rightIdx = BuildRecursive(triBounds, triCentroids,
                                                 mid, count - (mid - first),
                                                 leafSize, depth + 1);

    nodes_[myIdx].left     = leftIdx;
    nodes_[myIdx].right    = rightIdx;
    nodes_[myIdx].firstTri = 0;
    nodes_[myIdx].triCount = 0;
    return myIdx;
}

} // namespace koilo
