// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/mesh_bvh.hpp>

#include <koilo/assets/model/indexgroup.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace koilo {

namespace {

constexpr uint32_t kLeafTargetTris = 4;
constexpr int      kMaxDepth       = 32;

inline void ExpandAABB(Vector3D& bmin, Vector3D& bmax, const Vector3D& p) {
    if (p.X < bmin.X) bmin.X = p.X;
    if (p.Y < bmin.Y) bmin.Y = p.Y;
    if (p.Z < bmin.Z) bmin.Z = p.Z;
    if (p.X > bmax.X) bmax.X = p.X;
    if (p.Y > bmax.Y) bmax.Y = p.Y;
    if (p.Z > bmax.Z) bmax.Z = p.Z;
}

inline bool IntersectAABB(const Ray& ray,
                          const Vector3D& invDir,
                          const Vector3D& bmin,
                          const Vector3D& bmax,
                          float maxDistance) {
    float tx1 = (bmin.X - ray.origin.X) * invDir.X;
    float tx2 = (bmax.X - ray.origin.X) * invDir.X;
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);

    float ty1 = (bmin.Y - ray.origin.Y) * invDir.Y;
    float ty2 = (bmax.Y - ray.origin.Y) * invDir.Y;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    float tz1 = (bmin.Z - ray.origin.Z) * invDir.Z;
    float tz2 = (bmax.Z - ray.origin.Z) * invDir.Z;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    return tmax >= std::max(tmin, 0.0f) && tmin <= maxDistance;
}

constexpr float kTriEpsilon = 1e-7f;

// Inlined Möller-Trumbore.
inline bool RayTri(const Ray& ray,
                   const Vector3D& v0,
                   const Vector3D& v1,
                   const Vector3D& v2,
                   bool backfaceCulling,
                   float& tOut) {
    Vector3D edge1 = v1 - v0;
    Vector3D edge2 = v2 - v0;
    Vector3D h = ray.direction.CrossProduct(edge2);
    float a = edge1.DotProduct(h);
    if (backfaceCulling) {
        if (a < kTriEpsilon) return false;
    } else {
        if (std::fabs(a) < kTriEpsilon) return false;
    }
    float f = 1.0f / a;
    Vector3D s = ray.origin - v0;
    float u = f * s.DotProduct(h);
    if (u < 0.0f || u > 1.0f) return false;
    Vector3D q = s.CrossProduct(edge1);
    float v = f * ray.direction.DotProduct(q);
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = f * edge2.DotProduct(q);
    if (t <= kTriEpsilon) return false;
    tOut = t;
    return true;
}

} // namespace

void MeshBVH::Build(const Vector3D* vertices,
                    const IndexGroup* indices,
                    int triCount) {
    nodes_.clear();
    tri_.clear();
    centroids_.clear();
    sourceVertices_ = vertices;
    sourceIndices_  = indices;

    if (!vertices || !indices || triCount <= 0) return;

    tri_.resize(static_cast<size_t>(triCount));
    centroids_.resize(static_cast<size_t>(triCount));
    for (int i = 0; i < triCount; ++i) {
        tri_[i] = static_cast<uint32_t>(i);
        const IndexGroup& g = indices[i];
        const Vector3D& a = vertices[g.GetIndex(0)];
        const Vector3D& b = vertices[g.GetIndex(1)];
        const Vector3D& c = vertices[g.GetIndex(2)];
        centroids_[i] = Vector3D((a.X + b.X + c.X) / 3.0f,
                                 (a.Y + b.Y + c.Y) / 3.0f,
                                 (a.Z + b.Z + c.Z) / 3.0f);
    }

    // Reserve a typical upper bound (2N nodes) to avoid reallocations
    // mid-build that would invalidate references into nodes_.
    nodes_.reserve(static_cast<size_t>(triCount) * 2 + 1);

    Node root{};
    root.left  = 0;
    root.count = static_cast<uint32_t>(triCount);
    nodes_.push_back(root);

    Subdivide(0, vertices, indices, 0);
}

void MeshBVH::Subdivide(uint32_t nodeIdx,
                        const Vector3D* vertices,
                        const IndexGroup* indices,
                        int depth) {
    Node& node = nodes_[nodeIdx];

    // Compute node AABB and centroid bounds.
    const float kInf = std::numeric_limits<float>::infinity();
    Vector3D bmin( kInf,  kInf,  kInf);
    Vector3D bmax(-kInf, -kInf, -kInf);
    Vector3D cmin( kInf,  kInf,  kInf);
    Vector3D cmax(-kInf, -kInf, -kInf);
    for (uint32_t i = 0; i < node.count; ++i) {
        uint32_t triIdx = tri_[node.left + i];
        const IndexGroup& g = indices[triIdx];
        ExpandAABB(bmin, bmax, vertices[g.GetIndex(0)]);
        ExpandAABB(bmin, bmax, vertices[g.GetIndex(1)]);
        ExpandAABB(bmin, bmax, vertices[g.GetIndex(2)]);
        ExpandAABB(cmin, cmax, centroids_[triIdx]);
    }
    node.bmin = bmin;
    node.bmax = bmax;

    if (node.count <= kLeafTargetTris || depth >= kMaxDepth) {
        return; // Leaf
    }

    // Pick split axis = longest centroid extent.
    Vector3D extent = cmax - cmin;
    int axis = 0;
    if (extent.Y > extent.X) axis = 1;
    if (axis == 0 ? extent.Z > extent.X : extent.Z > extent.Y) axis = 2;

    // Degenerate centroid extent -> leaf.
    float axisExtent = (axis == 0) ? extent.X : (axis == 1) ? extent.Y : extent.Z;
    if (axisExtent <= 0.0f) return;

    // Median split on centroid coordinate.
    auto centroidCoord = [&](uint32_t triIdx) {
        const Vector3D& c = centroids_[triIdx];
        return axis == 0 ? c.X : axis == 1 ? c.Y : c.Z;
    };

    uint32_t first = node.left;
    uint32_t last  = node.left + node.count;
    uint32_t mid   = first + node.count / 2;
    std::nth_element(tri_.begin() + first,
                     tri_.begin() + mid,
                     tri_.begin() + last,
                     [&](uint32_t a, uint32_t b) {
                         return centroidCoord(a) < centroidCoord(b);
                     });

    uint32_t leftCount  = mid - first;
    uint32_t rightCount = last - mid;
    if (leftCount == 0 || rightCount == 0) return; // Couldn't split

    // Convert this node into an interior node referencing two children.
    Node leftChild{};
    leftChild.left  = first;
    leftChild.count = leftCount;
    Node rightChild{};
    rightChild.left  = mid;
    rightChild.count = rightCount;

    uint32_t leftIdx  = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back(leftChild);
    nodes_.push_back(rightChild);

    // Re-fetch reference (vector may have been reserved enough; we reserved up front).
    Node& self = nodes_[nodeIdx];
    self.left  = leftIdx;
    self.count = 0;

    Subdivide(leftIdx,     vertices, indices, depth + 1);
    Subdivide(leftIdx + 1, vertices, indices, depth + 1);
}

bool MeshBVH::Intersect(const Ray& ray,
                        float maxDistance,
                        bool backfaceCulling,
                        float& outDistance,
                        Vector3D& outPoint,
                        Vector3D& outNormal) const {
    if (nodes_.empty() || !sourceVertices_ || !sourceIndices_) return false;

    Vector3D invDir;
    auto safeInv = [](float d) {
        return std::fabs(d) > 1e-30f ? 1.0f / d : std::numeric_limits<float>::infinity();
    };
    invDir.X = safeInv(ray.direction.X);
    invDir.Y = safeInv(ray.direction.Y);
    invDir.Z = safeInv(ray.direction.Z);

    bool  hit          = false;
    float closestT     = maxDistance;
    uint32_t closestTri = 0;

    // Iterative DFS using a small fixed-size stack.
    uint32_t stack[64];
    int sp = 0;
    stack[sp++] = 0;

    while (sp > 0) {
        uint32_t idx = stack[--sp];
        const Node& n = nodes_[idx];
        if (!IntersectAABB(ray, invDir, n.bmin, n.bmax, closestT)) continue;

        if (n.count == 0) {
            // Interior - push both children.
            if (sp + 2 <= static_cast<int>(sizeof(stack) / sizeof(stack[0]))) {
                stack[sp++] = n.left;
                stack[sp++] = n.left + 1;
            }
            continue;
        }

        // Leaf: test triangles.
        for (uint32_t i = 0; i < n.count; ++i) {
            uint32_t triIdx = tri_[n.left + i];
            const IndexGroup& g = sourceIndices_[triIdx];
            const Vector3D& v0 = sourceVertices_[g.GetIndex(0)];
            const Vector3D& v1 = sourceVertices_[g.GetIndex(1)];
            const Vector3D& v2 = sourceVertices_[g.GetIndex(2)];

            float t;
            if (RayTri(ray, v0, v1, v2, backfaceCulling, t) && t < closestT) {
                closestT   = t;
                closestTri = triIdx;
                hit        = true;
            }
        }
    }

    if (!hit) return false;

    const IndexGroup& g = sourceIndices_[closestTri];
    const Vector3D& v0 = sourceVertices_[g.GetIndex(0)];
    const Vector3D& v1 = sourceVertices_[g.GetIndex(1)];
    const Vector3D& v2 = sourceVertices_[g.GetIndex(2)];
    outDistance = closestT;
    outPoint    = ray.GetPoint(closestT);
    outNormal   = (v1 - v0).CrossProduct(v2 - v0).Normal();
    return true;
}

} // namespace koilo
