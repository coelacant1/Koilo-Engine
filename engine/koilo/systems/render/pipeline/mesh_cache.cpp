// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file mesh_cache.cpp
 * @brief Shared GPU mesh cache – implementation.
 *
 * Builds interleaved position/normal/UV vertex arrays from engine Mesh
 * geometry and uploads them through the RHI device interface.  Tracks
 * mesh GPU versions to re-upload only when geometry has changed.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */

#include "mesh_cache.hpp"
#include "../rhi/rhi_device.hpp"
#include <koilo/systems/scene/mesh.hpp>
#include <koilo/assets/model/itrianglegroup.hpp>
#include <koilo/core/geometry/3d/triangle.hpp>
#include <koilo/assets/model/indexgroup.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/core/math/vector2d.hpp>
#include <vector>

namespace koilo::rhi {

// -- Lifetime -----------------------------------------------------------

MeshCache::MeshCache(IRHIDevice* device)
    : device_(device) {}

MeshCache::~MeshCache() { Clear(); }

// -- Query / upload -----------------------------------------------------

const CachedMesh* MeshCache::GetOrUpload(Mesh* mesh)
{
    if (!mesh)
        return nullptr;

    ITriangleGroup* tg = mesh->GetTriangleGroup();
    if (!tg)
        return nullptr;

    const uint32_t triCount = tg->GetTriangleCount();
    if (triCount == 0)
        return nullptr;

    const uintptr_t key     = reinterpret_cast<uintptr_t>(mesh);
    const uint32_t  version = mesh->GetGPUVersion();

    // Fast path – already cached and up-to-date.
    auto it = cache_.find(key);
    if (it != cache_.end() && it->second.version == version)
        return &it->second;

    // Build interleaved vertex array from mesh geometry.
    Triangle3D* triangles = tg->GetTriangles();
    const bool       hasUV     = mesh->HasUV();
    const Vector2D*  uvVerts   = hasUV ? mesh->GetUVVertices()  : nullptr;
    const IndexGroup* uvIndices = hasUV ? mesh->GetUVIndexGroup() : nullptr;

    std::vector<RHIVertex> verts;
    verts.reserve(triCount * 3);

    for (uint32_t i = 0; i < triCount; ++i) {
        const Triangle3D& tri = triangles[i];
        const Vector3D& p1 = *tri.p1;
        const Vector3D& p2 = *tri.p2;
        const Vector3D& p3 = *tri.p3;

        // Face normal from cross product.
        Vector3D n   = (p2 - p1).CrossProduct(p3 - p1);
        float    len = n.Magnitude();
        if (len > 1e-8f)
            n = n * (1.0f / len);

        float u0 = 0, v0 = 0, u1 = 0, v1 = 0, u2 = 0, v2 = 0;
        if (hasUV && uvIndices && uvVerts) {
            const IndexGroup& uvIdx = uvIndices[i];
            u0 = uvVerts[uvIdx.A].X;  v0 = uvVerts[uvIdx.A].Y;
            u1 = uvVerts[uvIdx.B].X;  v1 = uvVerts[uvIdx.B].Y;
            u2 = uvVerts[uvIdx.C].X;  v2 = uvVerts[uvIdx.C].Y;
        }

        verts.push_back({p1.X, p1.Y, p1.Z, n.X, n.Y, n.Z, u0, v0});
        verts.push_back({p2.X, p2.Y, p2.Z, n.X, n.Y, n.Z, u1, v1});
        verts.push_back({p3.X, p3.Y, p3.Z, n.X, n.Y, n.Z, u2, v2});
    }

    // Destroy stale buffer if re-uploading.
    if (it != cache_.end())
        device_->DestroyBuffer(it->second.vertexBuffer);

    // Create RHI buffer and upload vertex data.
    RHIBufferDesc desc{};
    desc.size        = verts.size() * sizeof(RHIVertex);
    desc.usage       = RHIBufferUsage::Vertex;
    desc.hostVisible = true;

    RHIBuffer buf = device_->CreateBuffer(desc);
    device_->UpdateBuffer(buf, verts.data(), desc.size, 0);

    CachedMesh entry{buf, static_cast<int>(verts.size()), version};
    cache_[key] = entry;
    return &cache_[key];
}

// -- Invalidation -------------------------------------------------------

void MeshCache::Invalidate(Mesh* mesh)
{
    if (!mesh)
        return;

    const uintptr_t key = reinterpret_cast<uintptr_t>(mesh);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        device_->DestroyBuffer(it->second.vertexBuffer);
        cache_.erase(it);
    }
}

void MeshCache::Clear()
{
    for (auto& [key, entry] : cache_)
        device_->DestroyBuffer(entry.vertexBuffer);
    cache_.clear();
}

// -- Accessors ----------------------------------------------------------

size_t MeshCache::Size() const { return cache_.size(); }

} // namespace koilo::rhi
