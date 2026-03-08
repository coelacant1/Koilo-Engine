// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @brief Morph target (blend shape) data for KoiloMesh
 */
struct MorphTarget {
    std::string name;
    std::vector<uint32_t> indices;      // Affected vertex indices
    std::vector<float> deltaX;          // X deltas
    std::vector<float> deltaY;          // Y deltas
    std::vector<float> deltaZ;          // Z deltas

    KL_BEGIN_FIELDS(MorphTarget)
        KL_FIELD(MorphTarget, name, "Name", 0, 0),
        KL_FIELD(MorphTarget, indices, "Indices", -2147483648, 2147483647),
        KL_FIELD(MorphTarget, deltaX, "Delta x", -2147483648, 2147483647),
        KL_FIELD(MorphTarget, deltaY, "Delta y", -2147483648, 2147483647),
        KL_FIELD(MorphTarget, deltaZ, "Delta z", -2147483648, 2147483647)
    KL_END_FIELDS

    KL_BEGIN_METHODS(MorphTarget)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(MorphTarget)
        /* No reflected ctors. */
    KL_END_DESCRIBE(MorphTarget)

};

/**
 * @brief Loads .kmesh binary files
 * 
 * Format:
 *   Header (32 bytes):
 *     [0-3]   Magic: "KOIM"
 *     [4-7]   Version: uint32
 *     [8-11]  Vertex count: uint32
 *     [12-15] Triangle count: uint32
 *     [16-19] Morph count: uint32
 *     [20-23] Flags: uint32
 *     [24-31] Reserved
 *   Vertices: count × 12 bytes (float x, y, z)
 *   Triangles: count × 12 bytes (uint32 v0, v1, v2)
 *   Morphs: (name + affected vertices + deltas)
 *   End: "ENDM" (4 bytes)
 */
class KoiloMeshLoader {
public:
    KoiloMeshLoader();
    ~KoiloMeshLoader();
    
    /**
     * @brief Load .kmesh file (auto-detects binary or ASCII)
     * @param filepath Path to .kmesh file
     * @return true if loaded successfully
     */
    bool Load(const char* filepath);
    
    /**
     * @brief Get loaded vertices
     * @return Pointer to vertex array (interleaved x,y,z floats)
     */
    const float* GetVertices() const { return vertices_.data(); }
    
    /**
     * @brief Get loaded triangles
     * @return Pointer to triangle indices (uint32 triplets)
     */
    const uint32_t* GetTriangles() const { return triangles_.data(); }
    
    /**
     * @brief Get number of vertices
     */
    uint32_t GetVertexCount() const { return vertexCount_; }
    
    /**
     * @brief Get number of triangles
     */
    uint32_t GetTriangleCount() const { return triangleCount_; }
    
    /**
     * @brief Get number of morph targets
     */
    uint32_t GetMorphCount() const { return static_cast<uint32_t>(morphs_.size()); }
    
    /**
     * @brief Get morph target by index
     * @param index Morph index (0 to GetMorphCount()-1)
     * @return Pointer to morph target, or nullptr if invalid
     */
    const MorphTarget* GetMorph(uint32_t index) const;
    
    /**
     * @brief Get morph target by name
     * @param name Morph name (case-sensitive)
     * @return Pointer to morph target, or nullptr if not found
     */
    const MorphTarget* GetMorph(const char* name) const;
    
    /**
     * @brief Get error message from last Load() call
     */
    const char* GetError() const { return error_.c_str(); }
    
    /**
     * @brief Check if file has UV coordinates
     */
    bool HasUVs() const { return hasUVs_; }
    
    /**
     * @brief Get UV coordinates (interleaved u,v floats)
     */
    const float* GetUVs() const { return uvs_.empty() ? nullptr : uvs_.data(); }

    /**
     * @brief Get UV triangle indices (uint32 triplets)
     */
    const uint32_t* GetUVTriangles() const { return uvTriangles_.empty() ? nullptr : uvTriangles_.data(); }

    /**
     * @brief Get number of UV vertices
     */
    uint32_t GetUVCount() const { return uvCount_; }

    /**
     * @brief Check if file has normals
     */
    bool HasNormals() const { return hasNormals_; }

private:
    bool LoadBinary(const char* filepath);
    bool LoadAscii(const char* filepath);
    std::vector<float> vertices_;       // Interleaved x,y,z
    std::vector<uint32_t> triangles_;   // Triangle indices (v0,v1,v2 triplets)
    std::vector<float> uvs_;            // Interleaved u,v
    std::vector<uint32_t> uvTriangles_; // UV triangle indices (uv0,uv1,uv2 triplets)
    std::vector<MorphTarget> morphs_;   // Morph targets
    
    uint32_t vertexCount_;
    uint32_t triangleCount_;
    uint32_t uvCount_;
    uint32_t morphCount_;
    bool hasUVs_;
    bool hasNormals_;
    
    std::string error_;
    
    // Binary format parsing
    bool ReadHeader(const uint8_t* data, size_t size, size_t& offset);
    bool ReadVertices(const uint8_t* data, size_t size, size_t& offset);
    bool ReadTriangles(const uint8_t* data, size_t size, size_t& offset);
    bool ReadMorphs(const uint8_t* data, size_t size, size_t& offset, uint32_t morphCount);
    bool ValidateEndMarker(const uint8_t* data, size_t size, size_t offset);

    KL_BEGIN_FIELDS(KoiloMeshLoader)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(KoiloMeshLoader)
        KL_METHOD_AUTO(KoiloMeshLoader, Load, "Load"),
        KL_METHOD_AUTO(KoiloMeshLoader, GetVertices, "Get vertices"),
        KL_METHOD_AUTO(KoiloMeshLoader, GetTriangles, "Get triangles"),
        KL_METHOD_AUTO(KoiloMeshLoader, GetVertexCount, "Get vertex count"),
        KL_METHOD_AUTO(KoiloMeshLoader, GetTriangleCount, "Get triangle count"),
        KL_METHOD_AUTO(KoiloMeshLoader, GetMorphCount, "Get morph count"),
        /* Get morph */ KL_METHOD_OVLD_CONST(KoiloMeshLoader, GetMorph, const MorphTarget *, uint32_t),
        /* Get morph */ KL_METHOD_OVLD_CONST(KoiloMeshLoader, GetMorph, const MorphTarget *, const char *),
        KL_METHOD_AUTO(KoiloMeshLoader, GetError, "Get error"),
        KL_METHOD_AUTO(KoiloMeshLoader, HasUVs, "Has uvs"),
        KL_METHOD_AUTO(KoiloMeshLoader, HasNormals, "Has normals")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(KoiloMeshLoader)
        KL_CTOR0(KoiloMeshLoader)
    KL_END_DESCRIBE(KoiloMeshLoader)

};

} // namespace koilo
