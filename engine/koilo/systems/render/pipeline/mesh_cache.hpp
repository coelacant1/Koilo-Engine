// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file mesh_cache.hpp
 * @brief Shared GPU mesh cache using the RHI abstraction layer.
 *
 * Converts engine Mesh geometry to interleaved vertex buffers on first use
 * and caches them so that both the OpenGL and Vulkan backends avoid
 * duplicating the Mesh -> GPU buffer upload logic.  Re-uploads automatically
 * when the mesh GPU version changes.
 *
 * @date 03/18/2026
 * @author Coela Can't
 */
#pragma once

#include "../rhi/rhi_types.hpp"
#include <unordered_map>
#include <cstdint>
#include "../../../registry/reflect_macros.hpp"

namespace koilo      { class Mesh; }
namespace koilo::rhi { class IRHIDevice; }

namespace koilo::rhi {

// -- Shared vertex format (32 bytes) ------------------------------------

struct RHIVertex {
    float px, py, pz;   // position
    float nx, ny, nz;   // normal (face normal from cross product)
    float u, v;          // texcoord
};

// -- Cached mesh entry --------------------------------------------------

struct CachedMesh {
    RHIBuffer vertexBuffer;
    int       vertexCount = 0;
    uint32_t  version     = 0;
};

// -- MeshCache ----------------------------------------------------------

class MeshCache {
public:
    explicit MeshCache(IRHIDevice* device);
    ~MeshCache();

    MeshCache(const MeshCache&)            = delete;
    MeshCache& operator=(const MeshCache&) = delete;

    /**
     * @brief  Return the cached mesh, uploading or re-uploading if the
     *         GPU version changed.
     * @param  mesh  Source engine mesh (may be nullptr).
     * @return Pointer to cached entry, or nullptr when the mesh is null
     *         or contains no triangles.
     */
    const CachedMesh* GetOrUpload(Mesh* mesh);

    /**
     * @brief Remove a single mesh from the cache and destroy its GPU buffer.
     * @param mesh  The engine mesh to invalidate.
     */
    void Invalidate(Mesh* mesh);

    /**
     * @brief Destroy every cached GPU buffer and clear the cache.
     */
    void Clear();

    /**
     * @brief Number of meshes currently held in the cache.
     */
    size_t Size() const;

private:
    IRHIDevice*                              device_;
    std::unordered_map<uintptr_t, CachedMesh> cache_;

    KL_BEGIN_FIELDS(MeshCache)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(MeshCache)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MeshCache)
        KL_CTOR(MeshCache, IRHIDevice*)
    KL_END_DESCRIBE(MeshCache)

};

} // namespace koilo::rhi
